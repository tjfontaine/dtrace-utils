/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *  Test %e, %E format printing.
 *
 * SECTION: Output Formatting/printf()
 *
 */

#pragma D option quiet

float f;
double d;

BEGIN
{
	printf("\n");

	printf("%%e = %e\n", f);
	printf("%%E = %E\n", f);

	printf("%%e = %e\n", d);
	printf("%%E = %E\n", d);


	exit(0);
}
