/*
 * Oracle Linux DTrace.
 * Copyright © 2006, 2017, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION: FBT provider return value offset verification test.
 *
 * SECTION: FBT Provider/Probe arguments
 */

#pragma D option quiet
#pragma D option statusrate=10ms

BEGIN
{
	self->traceme = 1;
}

fbt::SyS_ioctl:entry
{
	printf("Entering the function\n");
}

fbt::SyS_ioctl:return
{
	printf("The offset = %u\n", arg0);
	exit(0);
}
