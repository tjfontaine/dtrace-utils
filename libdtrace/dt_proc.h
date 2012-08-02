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
 * Copyright 2007 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef	_DT_PROC_H
#define	_DT_PROC_H

#pragma ident	"%Z%%M%	%I%	%E% SMI"

#include <rtld_db.h>
#include <dtrace.h>
#include <pthread.h>
#include <dt_list.h>

#ifdef	__cplusplus
extern "C" {
#endif

typedef struct dt_proc {
	dt_list_t dpr_list;		/* prev/next pointers for lru chain */
	struct dt_proc *dpr_hash;	/* next pointer for pid hash chain */
	dtrace_hdl_t *dpr_hdl;		/* back pointer to libdtrace handle */
	struct ps_prochandle *dpr_proc;	/* proc handle for libproc calls */
	char dpr_errmsg[BUFSIZ];	/* error message */
	rd_agent_t *dpr_rtld;		/* rtld handle for librtld_db calls */
	pthread_mutex_t dpr_lock;	/* lock for manipulating dpr_hdl */
	uint8_t dpr_tid_locked;		/* true if the control thread holds
					 * dpr_locked */
	pthread_cond_t dpr_cv;		/* cond for startup/stop/quit/done */
	pid_t dpr_pid;			/* pid of process */
	uint_t dpr_refs;		/* reference count */
	uint8_t dpr_stop;		/* stop mask: see flag bits below */
	uint8_t dpr_done;		/* done flag: ctl thread has exited */
	uint8_t dpr_usdt;		/* usdt flag: usdt initialized */
	uint8_t dpr_created;            /* proc flag: true if we created this
					   process, false if we grabbed it */
	pthread_t dpr_tid;		/* control thread (or zero if none) */
} dt_proc_t;

typedef struct dt_proc_notify {
	dt_proc_t *dprn_dpr;		/* process associated with the event */
	char dprn_errmsg[BUFSIZ];	/* error message */
	struct dt_proc_notify *dprn_next; /* next pointer */
} dt_proc_notify_t;

#define	DT_PROC_STOP_IDLE	0x01	/* idle on owner's stop request */
#define	DT_PROC_STOP_CREATE	0x02	/* wait on dpr_cv at process exec */
#define	DT_PROC_STOP_GRAB	0x04	/* wait on dpr_cv at process grab */
#define	DT_PROC_STOP_PREINIT	0x08	/* wait on dpr_cv at rtld preinit */
#define	DT_PROC_STOP_POSTINIT	0x10	/* wait on dpr_cv at rtld postinit */
#define	DT_PROC_STOP_MAIN	0x20	/* wait on dpr_cv at a.out`main() */

typedef struct dt_proc_hash {
	pthread_mutex_t dph_lock;	/* lock protecting dph_notify list */
	pthread_cond_t dph_cv;		/* cond for waiting for dph_notify */
	dt_proc_notify_t *dph_notify;	/* list of pending proc notifications */
	dt_list_t dph_lrulist;		/* list of dt_proc_t's in lru order */
	uint_t dph_lrulim;		/* limit on number of procs to hold */
	uint_t dph_lrucnt;		/* count of cached process handles */
	uint_t dph_hashlen;		/* size of hash chains array */
	dt_proc_t *dph_hash[1];		/* hash chains array */
} dt_proc_hash_t;

extern struct ps_prochandle *dt_proc_create(dtrace_hdl_t *,
    const char *, char *const *);

extern struct ps_prochandle *dt_proc_grab(dtrace_hdl_t *, pid_t);
extern void dt_proc_release(dtrace_hdl_t *, struct ps_prochandle *);
extern void dt_proc_continue(dtrace_hdl_t *, struct ps_prochandle *);
extern void dt_proc_lock(dtrace_hdl_t *, struct ps_prochandle *);
extern void dt_proc_unlock(dtrace_hdl_t *, struct ps_prochandle *);
extern dt_proc_t *dt_proc_lookup(dtrace_hdl_t *, struct ps_prochandle *, int);

extern void dt_proc_hash_create(dtrace_hdl_t *);
extern void dt_proc_hash_destroy(dtrace_hdl_t *);

#ifdef	__cplusplus
}
#endif

#endif	/* _DT_PROC_H */
