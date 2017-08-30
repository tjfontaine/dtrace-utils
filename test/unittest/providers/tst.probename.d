/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 * 	Simple profile provider test.
 * 	print the 'probename' field i.e. Current probe description's name
 *	field.
 *	Match expected output in tst.probename.d.out
 *
 * SECTION: profile Provider/tick-n probes;
 * 	Variables/Built-in Variables
 *
 */

#pragma D option quiet

BEGIN
{
	i = 0;
}

profile:::tick-1sec
{
	printf("probe name = %s", probename);
	exit (0);
}
