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
 * Copyright 2006 Oracle, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#pragma	ident	"%Z%%M%	%I%	%E% SMI"

/* @@xfail: depends on procfs.d */

/*
 * ASSERTION:
 * To print lwpsinfo_t variables and make sure it succeeds
 *
 * SECTION: Variables/Built-in Variables
 */

#pragma D option quiet

BEGIN
{
	printf("The current thread's pr_flag is %d\n", curlwpsinfo->pr_flag);
	printf("The current threads lwpid is %d\n", curlwpsinfo->pr_lwpid);
	printf("The current thread's internal address is %u\n",
			curlwpsinfo->pr_addr);
	printf("The current thread's wait addr for sleeping lwp is %u\n",
			curlwpsinfo->pr_wchan);
	printf("The current lwp stat is %d\n", curlwpsinfo->pr_state);
	printf("The printable character for pr_state %c\n",
		curlwpsinfo->pr_sname);
	printf("The syscall number = %d\n", curlwpsinfo->pr_syscall);
	exit (0);
}
