/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, 2012, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *	bcopy should copy from one memory location to another
 *
 * SECTION: Actions and Subroutines/alloca();
 * 	Actions and Subroutines/bcopy()
 *
 */

#pragma D option quiet


BEGIN
{
	ptr = alloca(sizeof (unsigned long));
	bcopy((void *)&`max_pfn, ptr, sizeof (unsigned long));
	ulongp = (unsigned long *)ptr;
	ret = (`max_pfn == *ulongp) ? 0 : 1;
}

tick-1
/ret == 0/
{
	exit(0);
}

tick-1
/ret == 1/
{
	printf("memory address contained 0x%x, expected 0x%x\n",
		*ulongp, `max_pfn);
	exit(1);
}
