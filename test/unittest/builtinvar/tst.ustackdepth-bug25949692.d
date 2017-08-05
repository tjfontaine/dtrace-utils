/*
 * Oracle Linux DTrace.
 * Copyright © 2017, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 * 	Ensure that ustackdepth is not always 1.
 *
 * SECTION: Variables/Built-in Variables
 */

#pragma D option quiet

syscall::ioctl:entry
/ustackdepth == 1/
{
	exit(1);
}

syscall::ioctl:entry
/ustackdepth > 1/
{
	exit(0);
}
