/*
 * Oracle Linux DTrace.
 * Copyright © 2006, 2017, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION: FBT provider function entry and exit test.
 *
 * SECTION: FBT Provider/Probe arguments
 */

#pragma D option quiet
#pragma D option statusrate=10ms

fbt::SyS_ioctl:entry
{
	printf("Entering the ioctl function\n");
}

fbt::SyS_ioctl:return
{
	printf("Returning from ioctl function\n");
	exit(0);
}
