/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *  Test %g, %G format printing.
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

	printf("%%g = %g\n", f);
	printf("%%g = %g\n", d);

	printf("%%G = %G\n", f);
	printf("%%G = %G\n", d);


	exit(0);
}
