/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/* @@xfail: no msgsize or msgdsize on Linux */

/*
 * ASSERTION:
 * 	Make sure that the msgsize safe to use at every fbt probe
 *
 * SECTION: Actions and Subroutines/msgsize();
 *	Options and Tunables/bufsize;
 * 	Options and Tunables/bufpolicy;
 * 	Options and Tunables/statusrate
 */

#pragma D option bufsize=1000
#pragma D option bufpolicy=ring
#pragma D option statusrate=10ms

fbt:::
{
	on = (timestamp / 1000000000) & 1;
}

fbt:::
/on/
{
	trace(msgsize((mblk_t *)rand()));
}

fbt:::
/on/
{
	trace(msgdsize((mblk_t *)rand()));
}

tick-1sec
/n++ == 20/
{
	exit(0);
}
