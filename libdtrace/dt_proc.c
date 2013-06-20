/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */

/*
 * Copyright 2010 -- 2013 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * DTrace Process Control
 *
 * This library provides several mechanisms in the libproc control layer:
 *
 * Process Control: a control thread is created for each process to provide
 * callbacks on process exit, to handle ptrace()-related signal dispatch tasks,
 * and to provide a central point that all ptrace()-related requests from the
 * rest of DTrace can flow through, working around the limitation that ptrace()
 * is per-thread and that libproc makes extensive use of it.
 *
 * MT-Safety: due to the above ptrace() limitations, libproc is not MT-Safe or
 * even capable of multithreading, so a marshalling layer is provided to
 * route all communication with libproc through the control thread.
 *
 * NOTE: MT-Safety is NOT provided for libdtrace itself, or for use of the
 * dtrace_proc_grab/dtrace_proc_create mechanisms.  Like all exported libdtrace
 * calls, these are assumed to be MT-Unsafe.  MT-Safety is ONLY provided for
 * calls via the libproc marshalling layer.
 *
 * The ps_prochandles themselves are maintained along with a dt_proc_t struct in
 * a hash table indexed by PID.  This provides basic locking and reference
 * counting.  The dt_proc_t is also maintained in LRU order on dph_lrulist.  The
 * dph_lrucnt and dph_lrulim count the number of processes we have grabbed or
 * created but not retired, and the current limit on the number of actively
 * cached entries.
 *
 * The control threads currently invoke processes, resume them when
 * dt_proc_continue() is called, manage ptrace()-related signal dispatch and
 * breakpoint handling tasks, handle libproc requests from the rest of DTrace
 * relating to their specific process, and notify interested parties when the
 * process dies.
 *
 * A simple notification mechanism is provided for libdtrace clients using
 * dtrace_handle_proc() for notification of process death.  When this event
 * occurs, the dt_proc_t itself is enqueued on a notification list and the
 * control thread broadcasts to dph_cv.  dtrace_sleep() will wake up using this
 * condition and will then call the client handler as necessary.
 */

#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <errno.h>
#include <port.h>
#include <poll.h>

#include <mutex.h>

#include <dt_proc.h>
#include <dt_pid.h>
#include <dt_impl.h>

static int dt_proc_attach(dt_proc_t *dpr, int before_continue);
static void dt_proc_remove(dtrace_hdl_t *dtp, pid_t pid);
static void dt_proc_dpr_lock(dt_proc_t *dpr);
static void dt_proc_dpr_unlock(dt_proc_t *dpr);
static void dt_proc_ptrace_lock(struct ps_prochandle *P, void *arg,
    int ptracing);

static void
dt_proc_notify(dtrace_hdl_t *dtp, dt_proc_hash_t *dph, dt_proc_t *dpr,
    const char *msg)
{
	dt_proc_notify_t *dprn = dt_alloc(dtp, sizeof (dt_proc_notify_t));

	if (dprn == NULL) {
		dt_dprintf("failed to allocate notification for %d %s\n",
		    (int)dpr->dpr_pid, msg);
	} else {
		dprn->dprn_dpr = dpr;
		if (msg == NULL)
			dprn->dprn_errmsg[0] = '\0';
		else
			(void) strlcpy(dprn->dprn_errmsg, msg,
			    sizeof (dprn->dprn_errmsg));

		(void) pthread_mutex_lock(&dph->dph_lock);

		dprn->dprn_next = dph->dph_notify;
		dph->dph_notify = dprn;

		(void) pthread_cond_broadcast(&dph->dph_cv);
		(void) pthread_mutex_unlock(&dph->dph_lock);
	}
}

/*
 * Check to see if the control thread was requested to stop when the victim
 * process reached a particular event (why) rather than continuing the victim.
 * If 'why' is set in the stop mask, we wait on dpr_cv for dt_proc_continue().
 * If 'why' is not set, this function returns immediately and does nothing.
 */
static void
dt_proc_stop(dt_proc_t *dpr, uint8_t why)
{
	assert(MUTEX_HELD(&dpr->dpr_lock));
	assert(dpr->dpr_lock_holder == pthread_self());
	assert(why != DT_PROC_STOP_IDLE);

	if (dpr->dpr_stop & why) {
		unsigned long lock_count;

		dpr->dpr_stop |= DT_PROC_STOP_IDLE;
		dpr->dpr_stop &= ~why;

		pthread_cond_broadcast(&dpr->dpr_cv);

		lock_count = dpr->dpr_lock_count;
		dpr->dpr_lock_count = 0;
		dpr->dpr_cond_waiting = B_TRUE;

		while (dpr->dpr_stop & DT_PROC_STOP_IDLE)
			pthread_cond_wait(&dpr->dpr_cv, &dpr->dpr_lock);

		dpr->dpr_cond_waiting = B_FALSE;
		dpr->dpr_lock_holder = pthread_self();
		dpr->dpr_lock_count = lock_count;
		dpr->dpr_stop |= DT_PROC_STOP_RESUMING;

		dt_dprintf("%p: dt_proc_stop(), control thread now waiting "
		    "for resume.\n", (int) dpr->dpr_pid);
	}
}

/*
 * After a stop is carried out and we have carried out any operations that must
 * be done serially, we must signal back to the process waiting in
 * dt_proc_continue() that it can resume.
 */
static void
dt_proc_resume(dt_proc_t *dpr)
{
	assert(MUTEX_HELD(&dpr->dpr_lock));
	assert(dpr->dpr_lock_holder == pthread_self());

	dt_dprintf("dt_proc_resume(), dpr_stop: %x (%i)\n",
	    dpr->dpr_stop, dpr->dpr_stop & DT_PROC_STOP_RESUMING);

	if (dpr->dpr_stop & DT_PROC_STOP_RESUMING) {
		dpr->dpr_stop &= ~DT_PROC_STOP_RESUMING;
		dpr->dpr_stop |= DT_PROC_STOP_RESUMED;
		pthread_cond_broadcast(&dpr->dpr_cv);

		dt_dprintf("dt_proc_resume(), control thread resumed. Lock count: %i\n",
			dpr->dpr_lock_count);
	}
}

/*
 * Fire a one-shot breakpoint to say that the child has got to an interesting
 * place from which we should grab control, possibly blocking.
 *
 * The dt_proc_lock is already held when this function is called.
 */
static int
dt_break_interesting(uintptr_t addr, void *dpr_data)
{
	dt_proc_t *dpr = dpr_data;

	dt_dprintf("pid %d: breakpoint on interesting locus\n", (int)dpr->dpr_pid);

	dt_proc_stop(dpr, dpr->dpr_hdl->dt_prcmode);
	dt_proc_resume(dpr);
	Punbkpt(dpr->dpr_proc, addr);

	return PS_RUN;
}

/*
 * A one-shot breakpoint that fires at a point at which the dynamic linker has
 * initialized far enough to enable us to do reliable symbol lookups, and thus
 * drop a breakpoint on main().
 *
 * The dt_proc_lock is already held when this function is called.
 */
static int
dt_break_drop_main(uintptr_t addr, void *dpr_data)
{
	dt_proc_t *dpr = dpr_data;
	int ret;

	dt_dprintf("pid %d: breakpoint on main()\n", (int)dpr->dpr_pid);

	ret = dt_proc_attach(dpr, B_FALSE);
	Punbkpt(dpr->dpr_proc, addr);

	/*
	 * If we couldn't dt_proc_attach(), because we couldn't find main();
	 * rendezvous here, instead.
	 */
	if (ret < 0) {
		dt_dprintf("pid %d: main() lookup failed, resuming now\n", (int)dpr->dpr_pid);
		dt_proc_stop(dpr, dpr->dpr_hdl->dt_prcmode);
		dt_proc_resume(dpr);
	}

	return PS_RUN;
}

/*
 * Event handler invoked automatically from within Pwait() when an interesting
 * event occurs.
 *
 * The dt_proc_lock is already held when this function is called.
 */

static void
dt_proc_rdevent(rd_agent_t *rd, rd_event_msg_t *msg, void *state)
{
	dt_proc_t *dpr = state;
	dtrace_hdl_t *dtp = dpr->dpr_hdl;

	/*
	 * Ignore the state deallocation call.
	 */
	if (msg == NULL)
		return;

	dt_dprintf("pid %d: rtld event, type=%d state %d\n",
	    (int)dpr->dpr_pid, msg->type, msg->state);

	switch (msg->type) {
	case RD_DLACTIVITY:
		if (msg->state != RD_CONSISTENT)
			break;

		Pupdate_syms(dpr->dpr_proc);
		if (dt_pid_create_probes_module(dtp, dpr) != 0)
			dt_proc_notify(dtp, dtp->dt_procs, dpr,
			    dpr->dpr_errmsg);

		break;
	case RD_NONE:
		/* cannot happen, but do nothing anyway */
		break;
	}
}

/*
 * Aarrange to be notified whenever the set of shared libraries in the child is
 * updated.
 */
static void
dt_proc_rdagent(dt_proc_t *dpr)
{
	/*
	 * TODO: this doesn't yet cope with statically linked programs, for
	 * which rd_event_enable() will return RD_NOMAPS until the first
	 * dlopen() happens, who knows how late into the program's execution.
	 *
	 * All of these calls are basically free if the agent already exists
	 * and monitoring is already active.
	 */
        if (Prd_agent(dpr->dpr_proc) != NULL)
		rd_event_enable(Prd_agent(dpr->dpr_proc), dt_proc_rdevent, dpr);
}

/*
 * Possibly arrange to stop the process, post-attachment, at the right place.
 * This may be called twice, before the dt_proc_continue() rendezvous just in
 * case the dynamic linker is far enough up to help us out, and from a
 * breakpoint set on preinit otherwise.
 *
 * Returns 0 on success, or -1 on failure (in which case the process is
 * still halted).
 */
static int
dt_proc_attach(dt_proc_t *dpr, int before_continue)
{
	uintptr_t addr = 0;
	GElf_Sym main_sym;
	dtrace_hdl_t *dtp = dpr->dpr_hdl;
	int (*handler) (uintptr_t addr, void *data) = dt_break_interesting;

	assert(MUTEX_HELD(&dpr->dpr_lock));

	dt_proc_rdagent(dpr);

	/*
	 * If we're stopping on exec we have no breakpoints to drop: if
	 * we're stopping on preinit and it's after the dt_proc_continue()
	 * rendezvous, we've already dropped the necessary breakpoints.
	 */

	if (dtp->dt_prcmode == DT_PROC_STOP_CREATE)
		return 0;

	if (!before_continue && dtp->dt_prcmode == DT_PROC_STOP_PREINIT)
		return 0;

	if (before_continue)
		/*
		 * Before dt_proc_continue().  Preinit, postinit and main all
		 * get a breakpoint dropped on the process entry point, though
		 * postinit and main use a different handler.
		 */
		switch (dtp->dt_prcmode) {
		case DT_PROC_STOP_POSTINIT:
		case DT_PROC_STOP_MAIN:
			handler = dt_break_drop_main;
		case DT_PROC_STOP_PREINIT:
			addr = Pgetauxval(dpr->dpr_proc, AT_ENTRY);
		}
	else
		/*
		 * After dt_proc_continue().  Postinit and main get a breakpoint
		 * dropped on main().
		 */
		switch (dtp->dt_prcmode) {
		case DT_PROC_STOP_POSTINIT:
		case DT_PROC_STOP_MAIN:
			if (Pxlookup_by_name(dpr->dpr_proc, LM_ID_BASE,
				PR_OBJ_EVERY, "main", &main_sym, NULL) == 0)
				addr = main_sym.st_value;
		}

	if (addr &&
	    Pbkpt(dpr->dpr_proc, addr, B_FALSE, handler, NULL, dpr) == 0)
		return 0;

	dt_dprintf("Cannot drop breakpoint in child process: acting as if "
	    "evaltime=%s were in force.\n", before_continue ? "exec" :
	    "preinit");
	dpr->dpr_stop &= ~dtp->dt_prcmode;
	if (before_continue) {
		dpr->dpr_stop |= DT_PROC_STOP_CREATE;
		dtp->dt_prcmode = DT_PROC_STOP_CREATE;
	} else {
		dpr->dpr_stop |= DT_PROC_STOP_PREINIT;
		dtp->dt_prcmode = DT_PROC_STOP_PREINIT;
	}

	return -1;
}

/*PRINTFLIKE3*/
_dt_printflike_(3,4)
static struct ps_prochandle *
dt_proc_error(dtrace_hdl_t *dtp, dt_proc_t *dpr, const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	dt_set_errmsg(dtp, NULL, NULL, NULL, 0, format, ap);
	va_end(ap);

	(void) dt_set_errno(dtp, EDT_COMPILER);
	return (NULL);
}

/*
 * Proxies for Pwait() and ptrace() that route all calls via the control thread.
 *
 * Must be called under dpr_lock.
 */
static long
proxy_pwait(struct ps_prochandle *P, void *arg, boolean_t block)
{
	dt_proc_t *dpr = arg;
	char junk = '\0'; /* unimportant */
	unsigned long lock_count;

	assert(MUTEX_HELD(&dpr->dpr_lock));
	assert(dpr->dpr_lock_holder == pthread_self());

	/*
	 * If we are already in the right thread, pass the call
	 * straight on.
	 */
	if (dpr->dpr_tid == pthread_self())
		return Pwait_internal(P, block);

	/*
	 * Proxy it, signal the control thread, and wait for the response.
	 */

	dpr->dpr_proxy_rq = proxy_pwait;
	dpr->dpr_proxy_args.dpr_pwait.P = P;
	dpr->dpr_proxy_args.dpr_pwait.block = block;

	errno = 0;
	while (write(dpr->dpr_proxy_fd[1], &junk, 1) < 0 && errno == EINTR);
	if (errno != 0 && errno != EINTR) {
		dt_proc_error(dpr->dpr_hdl, dpr, "Cannot write to proxy pipe "
		    "for Pwait(), deadlock is certain: %s\n", strerror(errno));
		return (-1);
	}

	lock_count = dpr->dpr_lock_count;
	dpr->dpr_lock_count = 0;

	while (dpr->dpr_proxy_rq != NULL)
		pthread_cond_wait(&dpr->dpr_msg_cv, &dpr->dpr_lock);

	dpr->dpr_lock_holder = pthread_self();
	dpr->dpr_lock_count = lock_count;

	errno = dpr->dpr_proxy_errno;
	return dpr->dpr_proxy_ret;
}

static long
proxy_ptrace(enum __ptrace_request request, void *arg, pid_t pid, void *addr,
    void *data)
{
	dt_proc_t *dpr = arg;
	char junk = '\0'; /* unimportant */
	unsigned long lock_count;

	assert(MUTEX_HELD(&dpr->dpr_lock));
	assert(dpr->dpr_lock_holder == pthread_self());

	/*
	 * If we are already in the right thread, pass the call
	 * straight on.
	 */
	if (dpr->dpr_tid == pthread_self())
		return ptrace(request, pid, addr, data);

	/*
	 * Proxy it, signal the control thread, and wait for the response.
	 */

	dpr->dpr_proxy_rq = proxy_ptrace;
	dpr->dpr_proxy_args.dpr_ptrace.request = request;
	dpr->dpr_proxy_args.dpr_ptrace.pid = pid;
	dpr->dpr_proxy_args.dpr_ptrace.addr = addr;
	dpr->dpr_proxy_args.dpr_ptrace.data = data;

	errno = 0;
	while (write(dpr->dpr_proxy_fd[1], &junk, 1) < 0 && errno == EINTR);
	if (errno != 0 && errno != EINTR) {
		dt_proc_error(dpr->dpr_hdl, dpr, "Cannot write to proxy pipe "
		    "for Pwait(), deadlock is certain: %s\n", strerror(errno));
		return (-1);
	}

	lock_count = dpr->dpr_lock_count;
	dpr->dpr_lock_count = 0;

	while (dpr->dpr_proxy_rq != NULL)
		pthread_cond_wait(&dpr->dpr_msg_cv, &dpr->dpr_lock);

	dpr->dpr_lock_holder = pthread_self();
	dpr->dpr_lock_count = lock_count;

	errno = dpr->dpr_proxy_errno;
	return dpr->dpr_proxy_ret;
}

typedef struct dt_proc_control_data {
	dtrace_hdl_t *dpcd_hdl;			/* DTrace handle */
	dt_proc_t *dpcd_proc;			/* process to control */

	/*
	 * This pipe contains data only when the dt_proc.proxy_rq contains a
	 * proxy request that needs handling on behalf of DTrace's main thread.
	 * DTrace will be waiting for the response on the dpr_msg_cv.
	 */
	int dpcd_proxy_fd[2];

	/*
	 * The next two are only valid while the master thread is calling
	 * dt_proc_create(), and only useful when dpr_created is true.
	 */
	const char *dpcd_start_proc;
	char * const *dpcd_start_proc_argv;
} dt_proc_control_data_t;

static void dt_proc_control_cleanup(void *arg);

/*
 * Main loop for all victim process control threads.  We initialize all the
 * appropriate /proc control mechanisms, start the process and halt it, notify
 * the caller of this, then wait for the caller to indicate its readiness and
 * resume the process: only then do we enter a loop waiting for the process to
 * stop on an event or die.  We process any events by calling appropriate
 * subroutines, and exit when the victim dies or we lose control.
 *
 * The control thread synchronizes the use of dpr_proc with other libdtrace
 * threads using dpr_lock.  We hold the lock for all of our operations except
 * waiting while the process is running.  If the libdtrace client wishes to exit
 * or abort our wait, thread cancellation can be used.
 */
static void *
dt_proc_control(void *arg)
{
	dt_proc_control_data_t *datap = arg;
	dtrace_hdl_t *dtp = datap->dpcd_hdl;
	dt_proc_t *dpr = datap->dpcd_proc;
	dt_proc_hash_t *dph = dpr->dpr_hdl->dt_procs;
	int err;
	pid_t pid;
	struct ps_prochandle *P;
	struct pollfd pfd[2];

	/*
	 * Arrange to clean up when cancelled by dt_proc_destroy() on shutdown.
	 */
	pthread_cleanup_push(dt_proc_control_cleanup, dpr);

	/*
	 * Lock our mutex, preventing races between cv broadcasts to our
	 * controlling thread and dt_proc_continue() or process destruction.
	 */
	dt_proc_dpr_lock(dpr);

	dpr->dpr_proxy_fd[0] = datap->dpcd_proxy_fd[0];
	dpr->dpr_proxy_fd[1] = datap->dpcd_proxy_fd[1];

	/*
	 * Either create the process, or grab it.  Whichever, on failure, quit
	 * and let our cleanup run (signalling failure to
	 * dt_proc_create_thread() in the process).
	 *
	 * At this point, the process is halted at exec(), if created.
	 */

	if (dpr->dpr_created) {
		if ((dpr->dpr_proc = Pcreate(datap->dpcd_start_proc,
			    datap->dpcd_start_proc_argv, dpr, &err)) == NULL) {
			dt_proc_error(dtp, dpr, "failed to execute %s: %s\n",
			    datap->dpcd_start_proc, strerror(err));
			pthread_exit(NULL);
		}
		dpr->dpr_pid = Pgetpid(dpr->dpr_proc);
	} else {
		if ((dpr->dpr_proc = Pgrab(dpr->dpr_pid, dpr, &err)) == NULL) {
			dt_proc_error(dtp, dpr, "failed to grab pid %li: %s\n",
			    (long) dpr->dpr_pid, strerror(err));
			pthread_exit(NULL);
		}
	}

	/*
	 * Arrange to proxy Pwait() and ptrace() calls through the
	 * thread-spanning proxies.
	 */
	Pset_pwait_wrapper(dpr->dpr_proc, proxy_pwait);
	Pset_ptrace_wrapper(dpr->dpr_proc, proxy_ptrace);

	pid = dpr->dpr_pid;
	P = dpr->dpr_proc;

	/*
	 * Make a waitfd to this process, and set up polling structures
	 * appropriately.  WEXITED | WSTOPPED is what Pwait() waits for.
	 */
	if ((dpr->dpr_fd = waitfd(P_PID, dpr->dpr_pid, WEXITED | WSTOPPED, 0)) < 0) {
		dt_proc_error(dtp, dpr, "failed to get waitfd for pid %li: %s\n",
		    (long) dpr->dpr_pid, strerror(err));
		pthread_exit(NULL);
	}

	pfd[0].fd = dpr->dpr_fd;
	pfd[0].events = POLLIN;
	pfd[1].fd = dpr->dpr_proxy_fd[0];
	pfd[1].events = POLLIN;

	/*
	 * Enable rtld breakpoints at the location specified by dt_prcmode (or
	 * drop other breakpoints which will eventually enable us to drop
	 * breakpoints at that location).
	 */
	dt_proc_attach(dpr, B_TRUE);

	/*
	 * Wait for a rendezvous from dt_proc_continue().  After this point,
	 * datap and all that it points to is no longer valid.
	 *
	 * This covers evaltime=exec and grabs, but not the three evaltimes that
	 * depend on breakpoints.  Those wait for rendezvous from within the
	 * breakpoint handler, invoked from Pwait() below.
	 */
	dt_proc_stop(dpr, dpr->dpr_created ? DT_PROC_STOP_CREATE :
	    DT_PROC_STOP_GRAB);

	/*
	 * Set the process going, and notify the main thread that it is now safe
	 * to return from dt_proc_continue().
	 */

	Ptrace_set_detached(P, dpr->dpr_created);
	Puntrace(P, 0);
	dt_proc_resume(dpr);

	/*
	 * Wait for the process corresponding to this control thread to stop,
	 * process the event, and then set it running again.  We want to sleep
	 * with dpr_lock *unheld* so that other parts of libdtrace can send
	 * requests to us, which is protected by that lock.  It is impossible
	 * for them, or any thread but this one, to modify the Pstate(), so we
	 * can call that without grabbing the lock.
	 */
	for (;;) {
		dt_proc_dpr_unlock(dpr);

		while (errno = EINTR,
		    poll(pfd, 2, -1) <= 0 && errno == EINTR)
			continue;

		/*
		 * We can block for arbitrarily long periods on this lock if the
		 * main thread is in a Ptrace()/Puntrace() region, unblocking
		 * only briefly when requests come in from the process.  This
		 * will not introduce additional latencies because the process
		 * is generally halted at this point, and being frequently
		 * Pwait()ed on by libproc (which proxies back to here).
		 *
		 * Note that if the process state changes while the lock is
		 * taken out by the main thread, the main thread will often
		 * proceed to Pwait() on it.  The ordering of these next two
		 * block is therefore crucial: we must check for proxy requests
		 * from the main thread *before* we check for process state
		 * changes via Pwait(), because one of the proxy requests is a
		 * Pwait(), and the libproc in the main thread often wants to
		 * unblock only once that Pwait() has returned (possibly after
		 * running breakpoint handlers and the like, which will run in
		 * the control thread, with their effects visible in the main
		 * thread, all serialized by dpr_lock).
		 */
		dt_proc_dpr_lock(dpr);

		/*
		 * Incoming proxy request.  Drain this byte out of the pipe, and
		 * handle it.  (Multiple bytes cannot land on the pipe, so we
		 * don't need to consider this case -- but if they do, it is
		 * harmless, because the dpr_proxy_rq will be NULL in subsequent
		 * calls.)
		 */
		if (pfd[1].revents != 0) {
			char junk;
			read(dpr->dpr_proxy_fd[0], &junk, 1);
			pfd[1].revents = 0;

			if (dpr->dpr_proxy_rq == proxy_pwait) {
				dt_dprintf("%d: Handling a proxy Pwait(%i)\n",
				    pid, dpr->dpr_proxy_args.dpr_pwait.block);
				errno = 0;
				dpr->dpr_proxy_ret = proxy_pwait
				    (dpr->dpr_proxy_args.dpr_pwait.P, dpr,
					dpr->dpr_proxy_args.dpr_pwait.block);

			} else if (dpr->dpr_proxy_rq == proxy_ptrace) {
				dt_dprintf("%d: Handling a proxy ptrace()\n", pid);
				errno = 0;
				dpr->dpr_proxy_ret = proxy_ptrace
				    (dpr->dpr_proxy_args.dpr_ptrace.request,
					dpr,
					dpr->dpr_proxy_args.dpr_ptrace.pid,
					dpr->dpr_proxy_args.dpr_ptrace.addr,
					dpr->dpr_proxy_args.dpr_ptrace.data);
			} else
				dt_dprintf("%d: unknown libproc request\n",
				    pid);

			dpr->dpr_proxy_errno = errno;
			dpr->dpr_proxy_rq = NULL;
			pthread_cond_signal(&dpr->dpr_msg_cv);
		}

		/*
		 * The process needs attention. Pwait() for it (which will make
		 * the waitfd transition back to empty).
		 */
		if (pfd[0].revents != 0) {
			dt_dprintf("%d: Handling a process state change\n",
			    pid);
			pfd[0].revents = 0;
			Pwait(P, B_FALSE);

			/*
			 * If we don't yet have an rtld_db handle, try again to
			 * get one.  (ld.so can take arbitrarily long to get
			 * ready for this.)
			 */
			dt_proc_rdagent(dpr);

			switch (Pstate(P)) {
			case PS_STOP:

				/*
				 * If the process stops showing one of the
				 * events that we are tracing, perform the
				 * appropriate response.
				 *
				 * TODO: the stop() action may need some work
				 * here.
				 */
				break;

				/*
				 * If the libdtrace caller (as opposed to any
				 * other process) tries to debug a monitored
				 * process, it may lead to our waitpid()
				 * returning strange results.  Fail in this
				 * case, until we define a protocol for
				 * communicating the waitpid() results to the
				 * caller, or relinquishing control temporarily.
				 * FIXME.
				 */
			case PS_TRACESTOP:
				dt_dprintf("%d: trace stop, nothing we can do\n",
				    pid);
				break;

			case PS_DEAD:
				dt_dprintf("%d: proc died\n", pid);
				break;
			}

			if (Pstate(P) == PS_DEAD)
				break; /* all the way out */
		}
	}

	/*
	 * If the caller is still waiting in dt_proc_continue(), resume it.
	 */
	dt_proc_resume(dpr);

	/*
	 * We only get here if the monitored process has died.  Enqueue the
	 * dt_proc_t structure on the dt_proc_hash_t notification list, then
	 * clean up.
	 */
	pthread_mutex_lock(&dph->dph_destroy_lock);

	dpr->dpr_proc = NULL;
	dt_proc_notify(dtp, dph, dpr, NULL);

	pthread_cleanup_pop(1);

	dt_list_delete(&dph->dph_lrulist, dpr);
	dt_proc_remove(dtp, pid);
	/* dt_free(dtp, dpr); -- XXX temporarily diked out */

	pthread_mutex_unlock(&dph->dph_destroy_lock);

	return (NULL);
}

/*
 * Cleanup handler, called when a process control thread exits or is cancelled.
 */
static void
dt_proc_control_cleanup(void *arg)
{
	dt_proc_t *dpr = arg;

	/*
	 * Set dpr_done and clear dpr_tid to indicate that the control thread
	 * has exited, and notify any waiting thread in dt_proc_destroy() that
	 * we have successfully exited.  Clean up the libproc state.
	 *
	 * If we were cancelled while already holding the mutex, don't lock it
	 * again.  If we were cancelled while in a pthread_cond_wait(), the
	 * owner might be inaccurate, but the lock will be taken out anyway, so
	 * track this via dpr_cond_waiting.  (Only the control thread ever sets
	 * this.)
	 *
	 * Forcibly reset the lock count: we don't care about nested locks taken
	 * out by ptrace() wrappers above us in the call stack, since the whole
	 * thread is going away.
 	 */
	if((!dpr->dpr_cond_waiting) &&
	    (dpr->dpr_lock_count == 0 ||
		dpr->dpr_lock_holder != pthread_self())) {
		dt_proc_dpr_lock(dpr);
		dpr->dpr_lock_count = 1;
	} else if (dpr->dpr_cond_waiting) {
		/*
		 * Fix up the lock holder and count so that internal locks taken
		 * out by Prelease() don't go awry.
		 */
		dpr->dpr_lock_holder = pthread_self();
		dpr->dpr_lock_count = 1;
	}

	Prelease(dpr->dpr_proc, dpr->dpr_created);
	dpr->dpr_done = B_TRUE;
	dpr->dpr_tid = 0;

	/*
	 * fd closing must be done with some care.  The thread may be cancelled
	 * before any of these fds have been assigned!
	 */

	if (dpr->dpr_fd)
	    close(dpr->dpr_fd);

	if (dpr->dpr_proxy_fd[0])
	    close(dpr->dpr_proxy_fd[0]);

	if (dpr->dpr_proxy_fd[1])
	    close(dpr->dpr_proxy_fd[1]);

	pthread_cond_broadcast(&dpr->dpr_cv);

	/*
	 * Completely unlock the lock, no matter what its depth.
	 */

	dpr->dpr_lock_count = 0;
	pthread_mutex_unlock(&dpr->dpr_lock);
}

dt_proc_t *
dt_proc_lookup(dtrace_hdl_t *dtp, struct ps_prochandle *P, int remove)
{
	dt_proc_hash_t *dph = dtp->dt_procs;
	pid_t pid = Pgetpid(P);
	dt_proc_t *dpr, **dpp = &dph->dph_hash[pid & (dph->dph_hashlen - 1)];

	for (dpr = *dpp; dpr != NULL; dpr = dpr->dpr_hash) {
		if (dpr->dpr_pid == pid)
			break;
		else
			dpp = &dpr->dpr_hash;
	}

	assert(dpr != NULL);
	assert(dpr->dpr_proc == P);

	if (remove)
		*dpp = dpr->dpr_hash; /* remove from pid hash chain */

	return (dpr);
}

static void
dt_proc_remove(dtrace_hdl_t *dtp, pid_t pid)
{
	dt_proc_hash_t *dph = dtp->dt_procs;
	dt_proc_t *dpr, **dpp = &dph->dph_hash[pid & (dph->dph_hashlen - 1)];

	for (dpr = *dpp; dpr != NULL; dpr = dpr->dpr_hash) {
		if (dpr->dpr_pid == pid)
			break;
		else
			dpp = &dpr->dpr_hash;
	}
	if (dpr == NULL)
		return;

	*dpp = dpr->dpr_hash;
}

/*
 * Retirement of a process happens after a long period of nonuse, and serves to
 * reduce the OS impact of process management of such processes.  A virtually
 * unlimited number of processes may exist in retired state at any one time:
 * they come out of retirement automatically when they are used again.
 */
static void
dt_proc_retire(struct ps_prochandle *P)
{
	(void) Pclose(P);
}

/*
 * Determine if a process is retired.  Very cheap.
 */
static int
dt_proc_retired(struct ps_prochandle *P)
{
	return (!Phasfds(P));
}

static void
dt_proc_destroy(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_t *dpr = dt_proc_lookup(dtp, P, B_FALSE);
	dt_proc_hash_t *dph = dtp->dt_procs;
	dt_proc_notify_t *npr, **npp;

	/*
	 * Before we free the process structure, remove this dt_proc_t from the
	 * lookup hash, and then walk the dt_proc_hash_t's notification list
	 * and remove this dt_proc_t if it is enqueued.  If the dpr is already
	 * gone, do nothing.
	 */
	if (dpr == NULL)
		return;

	dt_dprintf("%s pid %d\n", dpr->dpr_created ? "killing" : "releasing",
		dpr->dpr_pid);

	pthread_mutex_lock(&dph->dph_lock);
	dt_proc_lookup(dtp, P, B_TRUE);
	npp = &dph->dph_notify;

	while ((npr = *npp) != NULL) {
		if (npr->dprn_dpr == dpr) {
			*npp = npr->dprn_next;
			dt_free(dtp, npr);
		} else {
			npp = &npr->dprn_next;
		}
	}

	pthread_mutex_unlock(&dph->dph_lock);
	if (dpr->dpr_tid) {
		/*
		 * If the process is currently waiting in dt_proc_stop(), poke it
		 * into running again.  This is essential to ensure that the
		 * thread cancel does not hit while it already has the mutex
		 * locked.
		 */
		if (dpr->dpr_stop & DT_PROC_STOP_IDLE) {
			dpr->dpr_stop &= ~DT_PROC_STOP_IDLE;
			pthread_cond_broadcast(&dpr->dpr_cv);
		}

		/*
		 * Cancel the daemon thread, then wait for dpr_done to indicate
		 * the thread has exited.  (This will also terminate the
		 * process.)
		 *
		 * Do not use P below this point: it has been freed by the
		 * daemon's cleanup process.
		 */
		pthread_mutex_lock(&dpr->dpr_lock);
		pthread_cancel(dpr->dpr_tid);

		while (!dpr->dpr_done)
			(void) pthread_cond_wait(&dpr->dpr_cv, &dpr->dpr_lock);

		pthread_mutex_unlock(&dpr->dpr_lock);
	} else
		/*
		 * No daemon thread to clean up for us. Prelease() the
		 * underlying process explicitly.
		 */
		Prelease(P, dpr->dpr_created);

	assert(dph->dph_lrucnt != 0);
	dph->dph_lrucnt--;
	dt_list_delete(&dph->dph_lrulist, dpr);
	dt_proc_remove(dtp, dpr->dpr_pid);
	dt_free(dtp, dpr);
}

static int
dt_proc_create_thread(dtrace_hdl_t *dtp, dt_proc_t *dpr, uint_t stop,
    const char *file, char *const *argv)
{
	dt_proc_control_data_t data;
	sigset_t nset, oset;
	pthread_attr_t a;
	int err;

	(void) pthread_mutex_lock(&dpr->dpr_lock);
	dpr->dpr_stop |= stop; /* set bit for initial rendezvous */

	(void) pthread_attr_init(&a);
	(void) pthread_attr_setdetachstate(&a, PTHREAD_CREATE_DETACHED);

	(void) sigfillset(&nset);
	(void) sigdelset(&nset, SIGABRT);	/* unblocked for assert() */

	data.dpcd_hdl = dtp;
	data.dpcd_proc = dpr;
	data.dpcd_start_proc = file;
	data.dpcd_start_proc_argv = argv;

	if (pipe(data.dpcd_proxy_fd) < 0) {
		err = errno;
		(void) dt_proc_error(dpr->dpr_hdl, dpr,
		    "failed to create communicating pipe for pid %d: %s\n",
		    (int)dpr->dpr_pid, strerror(err));

		(void) pthread_mutex_unlock(&dpr->dpr_lock);
		(void) pthread_attr_destroy(&a);
		return (err);
	}

	Pset_ptrace_lock_hook(dt_proc_ptrace_lock);

	(void) pthread_sigmask(SIG_SETMASK, &nset, &oset);
	err = pthread_create(&dpr->dpr_tid, &a, dt_proc_control, &data);
	(void) pthread_sigmask(SIG_SETMASK, &oset, NULL);

	/*
	 * If the control thread was created, then wait on dpr_cv for either
	 * dpr_done to be set (the victim died or the control thread failed) or
	 * DT_PROC_STOP_IDLE to be set, indicating that the victim is now
	 * stopped and the control thread is at the rendezvous event.  On
	 * success, we return with the process and control thread stopped: the
	 * caller can then apply dt_proc_continue() to resume both.
	 */
	if (err == 0) {
		while (!dpr->dpr_done && !(dpr->dpr_stop & DT_PROC_STOP_IDLE))
			(void) pthread_cond_wait(&dpr->dpr_cv, &dpr->dpr_lock);

		dpr->dpr_lock_holder = pthread_self();

		/*
		 * If dpr_done is set, the control thread aborted before it
		 * reached the rendezvous event.  We try to provide a small
		 * amount of useful information to help figure out why.
		 */
		if (dpr->dpr_done) {
			err = ESRCH; /* cause grab() or create() to fail */
		}
	} else {
		(void) dt_proc_error(dpr->dpr_hdl, dpr,
		    "failed to create control thread for process-id %d: %s\n",
		    (int)dpr->dpr_pid, strerror(err));
	}

	(void) pthread_mutex_unlock(&dpr->dpr_lock);
	(void) pthread_attr_destroy(&a);

	if (err != 0)
		dt_free(dtp, dpr);

	return (err);
}

struct ps_prochandle *
dt_proc_create(dtrace_hdl_t *dtp, const char *file, char *const *argv,
	int flags)
{
	dt_proc_hash_t *dph = dtp->dt_procs;
	dt_proc_t *dpr;
	pthread_mutexattr_t attr;
	pthread_mutexattr_t *attrp = NULL;

	if ((dpr = dt_zalloc(dtp, sizeof (dt_proc_t))) == NULL)
		return (NULL); /* errno is set for us */

	if (_dtrace_debug_assert & DT_DEBUG_MUTEXES) {
		attrp = &attr;
		pthread_mutexattr_init(attrp);
		pthread_mutexattr_settype(attrp, PTHREAD_MUTEX_ERRORCHECK);
	}

	(void) pthread_mutex_init(&dpr->dpr_lock, attrp);
	(void) pthread_cond_init(&dpr->dpr_cv, NULL);
	(void) pthread_cond_init(&dpr->dpr_msg_cv, NULL);

	if (_dtrace_debug_assert & DT_DEBUG_MUTEXES) {
		pthread_mutexattr_destroy(attrp);
		attrp = NULL;
	}

	dpr->dpr_hdl = dtp;
	dpr->dpr_created = B_TRUE;

	if (dt_proc_create_thread(dtp, dpr, dtp->dt_prcmode,
		file, argv) != 0) {

		pthread_cond_destroy(&dpr->dpr_cv);
		pthread_cond_destroy(&dpr->dpr_msg_cv);
		pthread_mutex_destroy(&dpr->dpr_lock);
		dt_free(dtp, dpr);

		return (NULL); /* dt_proc_error() has been called for us */
	}

	dph->dph_lrucnt++;
	dpr->dpr_hash = dph->dph_hash[dpr->dpr_pid & (dph->dph_hashlen - 1)];
	dph->dph_hash[dpr->dpr_pid & (dph->dph_hashlen - 1)] = dpr;
	dt_list_prepend(&dph->dph_lrulist, dpr);

	dt_dprintf("created pid %d\n", (int)dpr->dpr_pid);
	dpr->dpr_refs++;

	/*
	 * If requested, wait for the control thread to finish initialization
	 * and rendezvous.
	 */
	if (flags & DTRACE_PROC_WAITING)
		dt_proc_continue(dtp, dpr->dpr_proc);

	return (dpr->dpr_proc);
}

struct ps_prochandle *
dt_proc_grab(dtrace_hdl_t *dtp, pid_t pid, int flags)
{
	dt_proc_hash_t *dph = dtp->dt_procs;
	uint_t h = pid & (dph->dph_hashlen - 1);
	dt_proc_t *dpr, *opr;
	pthread_mutexattr_t attr;
	pthread_mutexattr_t *attrp = NULL;

	/*
	 * Search the hash table for the pid.  If it is already grabbed or
	 * created, move the handle to the front of the lrulist, increment
	 * the reference count, and return the existing ps_prochandle.
	 *
	 * If it is retired, bring it out of retirement aggressively, so as to
	 * ensure that dph_lrucnt and dt_proc_retired() do not get out of synch
	 * (which would cause aggressive early retirement of processes even when
	 * unnecessary).
	 */
	for (dpr = dph->dph_hash[h]; dpr != NULL; dpr = dpr->dpr_hash) {
		if (dpr->dpr_pid == pid) {
			dt_dprintf("grabbed pid %d (cached)\n", (int)pid);
			dt_list_delete(&dph->dph_lrulist, dpr);
			dt_list_prepend(&dph->dph_lrulist, dpr);
			dpr->dpr_refs++;
			if (dt_proc_retired(dpr->dpr_proc)) {
				/* not retired any more */
				(void) Pmemfd(dpr->dpr_proc);
				dph->dph_lrucnt++;
			}
			return (dpr->dpr_proc);
		}
	}

	if ((dpr = dt_zalloc(dtp, sizeof (dt_proc_t))) == NULL)
		return (NULL); /* errno is set for us */

	if (_dtrace_debug_assert & DT_DEBUG_MUTEXES) {
		attrp = &attr;
		pthread_mutexattr_init(attrp);
		pthread_mutexattr_settype(attrp, PTHREAD_MUTEX_ERRORCHECK);
	}

	(void) pthread_mutex_init(&dpr->dpr_lock, attrp);
	(void) pthread_cond_init(&dpr->dpr_cv, NULL);
	(void) pthread_cond_init(&dpr->dpr_msg_cv, NULL);

	if (_dtrace_debug_assert & DT_DEBUG_MUTEXES) {
		pthread_mutexattr_destroy(attrp);
		attrp = NULL;
	}

	dpr->dpr_hdl = dtp;
	dpr->dpr_pid = pid;
	dpr->dpr_created = B_FALSE;

	/*
	 * Create a control thread for this process and store its ID in
	 * dpr->dpr_tid.
	 */
	if (dt_proc_create_thread(dtp, dpr, DT_PROC_STOP_GRAB, NULL, NULL) != 0) {

		pthread_cond_destroy(&dpr->dpr_cv);
		pthread_mutex_destroy(&dpr->dpr_lock);
		dt_free(dtp, dpr);

		return (NULL); /* dt_proc_error() has been called for us */
	}

	/*
	 * If we're currently caching more processes than dph_lrulim permits,
	 * attempt to find the least-recently-used process that is currently
	 * unreferenced and has not already been retired, and retire it.  (This
	 * does not actually delete it, because its presence is still necessary
	 * to ensure that we do put it into halted state again.  It merely
	 * closes any associated filehandles.)
	 *
	 * We know this expiry run cannot affect the handle currently being
	 * grabbed, or we'd have boosted its refcnt and returned already.
	 */
	if (dph->dph_lrucnt >= dph->dph_lrulim) {
		for (opr = dt_list_prev(&dph->dph_lrulist);
		     opr != NULL; opr = dt_list_prev(opr)) {
			if (opr->dpr_refs == 0 && !dt_proc_retired(opr->dpr_proc)) {
				dt_proc_retire(opr->dpr_proc);
				dph->dph_lrucnt--;
				break;
			}
		}
	}

	dph->dph_lrucnt++;
	dpr->dpr_hash = dph->dph_hash[h];
	dph->dph_hash[h] = dpr;
	dt_list_prepend(&dph->dph_lrulist, dpr);

	dt_dprintf("grabbed pid %d\n", (int)pid);
	dpr->dpr_refs++;

	/*
	 * If requested, wait for the control thread to finish initialization
	 * and rendezvous.
	 */
	if (flags & DTRACE_PROC_WAITING)
		dt_proc_continue(dtp, dpr->dpr_proc);

	return (dpr->dpr_proc);
}

void
dt_proc_release(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_t *dpr = dt_proc_lookup(dtp, P, B_FALSE);
	dt_proc_hash_t *dph = dtp->dt_procs;

	assert(dpr != NULL);
	assert(dpr->dpr_refs != 0);

	if (--dpr->dpr_refs == 0 &&
	    (dph->dph_lrucnt > dph->dph_lrulim) &&
	    !dt_proc_retired(dpr->dpr_proc)) {
		dt_proc_retire(P);
		dph->dph_lrucnt--;
	}
}

void
dt_proc_continue(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_t *dpr = dt_proc_lookup(dtp, P, B_FALSE);

	(void) pthread_mutex_lock(&dpr->dpr_lock);

	dt_dprintf("%i: doing a dt_proc_continue().\n", dpr->dpr_pid);

	/*
	 * A continue has two phases.  First, we broadcast to tell the control
	 * thread to awaken its child; then we wait for its signal to tell us
	 * that it has completed detaching that child.  Without this, we may
	 * grab the dpr_lock before it can be re-grabbed by the control thread
	 * and used to detach, leading to unbalanced Ptrace()/Puntrace() calls,
	 * a child permanently stuck in PS_TRACESTOP, and a rapid deadlock.
	 *
	 * This can only be called once for a given process: once the process
	 * has been resumed, that's it.
	 */

	if (dpr->dpr_stop & DT_PROC_STOP_RESUMED) {
		dt_dprintf("%i: Already resumed, returning.\n",
			dpr->dpr_pid);
		return;
	}

	if (dpr->dpr_stop & DT_PROC_STOP_IDLE) {
 		dpr->dpr_stop &= ~DT_PROC_STOP_IDLE;
		(void) pthread_cond_broadcast(&dpr->dpr_cv);
	}

	while (!(dpr->dpr_stop & DT_PROC_STOP_RESUMED))
		pthread_cond_wait(&dpr->dpr_cv, &dpr->dpr_lock);

	dt_dprintf("%i: dt_proc_continue()d.\n", dpr->dpr_pid);

	(void) pthread_mutex_unlock(&dpr->dpr_lock);
}

void
dt_proc_lock(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_dpr_lock(dt_proc_lookup(dtp, P, B_FALSE));
}

void
dt_proc_unlock(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_dpr_unlock(dt_proc_lookup(dtp, P, B_FALSE));
}

static void
dt_proc_dpr_lock(dt_proc_t *dpr)
{
	dt_dprintf("LOCK: depth %i, by %llx\n", dpr->dpr_lock_count,
	    pthread_self());

	if (dpr->dpr_lock_holder != pthread_self() ||
	    dpr->dpr_lock_count == 0) {
		dt_dprintf("Taking out lock\n");
		pthread_mutex_lock(&dpr->dpr_lock);
		dpr->dpr_lock_holder = pthread_self();
	}

	assert(MUTEX_HELD(&dpr->dpr_lock));
	assert(dpr->dpr_lock_count == 0 ||
	    dpr->dpr_lock_holder == pthread_self());

	dpr->dpr_lock_count++;
}

static void
dt_proc_dpr_unlock(dt_proc_t *dpr)
{
	int err;

	assert(dpr->dpr_lock_holder == pthread_self() &&
	    dpr->dpr_lock_count > 0);
	dpr->dpr_lock_count--;

	dt_dprintf("UNLOCK: depth %i, by %llx\n", dpr->dpr_lock_count,
	    pthread_self());
	if (dpr->dpr_lock_count == 0) {
		dt_dprintf("Relinquishing lock\n");
		err = pthread_mutex_unlock(&dpr->dpr_lock);
		assert(err == 0); /* check for unheld lock */
	} else
		assert(MUTEX_HELD(&dpr->dpr_lock));
}

static void
dt_proc_ptrace_lock(struct ps_prochandle *P, void *arg, int ptracing)
{
	dt_proc_t *dpr = arg;

	if (ptracing)
		dt_proc_dpr_lock(dpr);
	else
		dt_proc_dpr_unlock(dpr);
}

void
dt_proc_hash_create(dtrace_hdl_t *dtp)
{
	if ((dtp->dt_procs = dt_zalloc(dtp, sizeof (dt_proc_hash_t) +
	    sizeof (dt_proc_t *) * _dtrace_pidbuckets - 1)) != NULL) {

		(void) pthread_mutex_init(&dtp->dt_procs->dph_lock, NULL);
		(void) pthread_mutex_init(&dtp->dt_procs->dph_destroy_lock, NULL);
		(void) pthread_cond_init(&dtp->dt_procs->dph_cv, NULL);

		dtp->dt_procs->dph_hashlen = _dtrace_pidbuckets;
		dtp->dt_procs->dph_lrulim = _dtrace_pidlrulim;
	}
}

void
dt_proc_hash_destroy(dtrace_hdl_t *dtp)
{
	dt_proc_hash_t *dph = dtp->dt_procs;
	dt_proc_t *dpr;

	pthread_mutex_lock(&dph->dph_destroy_lock);
	while ((dpr = dt_list_next(&dph->dph_lrulist)) != NULL)
		dt_proc_destroy(dtp, dpr->dpr_proc);
	pthread_mutex_unlock(&dph->dph_destroy_lock);

	dtp->dt_procs = NULL;
	dt_free(dtp, dph);
}

struct ps_prochandle *
dtrace_proc_create(dtrace_hdl_t *dtp, const char *file, char *const *argv,
	int flags)
{
	dt_ident_t *idp = dt_idhash_lookup(dtp->dt_macros, "target");
	struct ps_prochandle *P = dt_proc_create(dtp, file, argv, flags);

	if (P != NULL && idp != NULL && idp->di_id == 0)
		idp->di_id = Pgetpid(P); /* $target = created pid */

	return (P);
}

struct ps_prochandle *
dtrace_proc_grab(dtrace_hdl_t *dtp, pid_t pid, int flags)
{
	dt_ident_t *idp = dt_idhash_lookup(dtp->dt_macros, "target");
	struct ps_prochandle *P = dt_proc_grab(dtp, pid, flags);

	if (P != NULL && idp != NULL && idp->di_id == 0)
		idp->di_id = pid; /* $target = grabbed pid */

	return (P);
}

void
dtrace_proc_release(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_release(dtp, P);
}

void
dtrace_proc_continue(dtrace_hdl_t *dtp, struct ps_prochandle *P)
{
	dt_proc_continue(dtp, P);
}
