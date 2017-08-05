/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/* @@note: wild negative numbers, tst.basics.d.out does not exist: validate */

/*
 * ASSERTION:
 * 	Simple Arithmetic expressions.
 *	Call simple expressions and make sure test succeeds.
 *	Match expected output in tst.basics.d.out
 *
 * SECTION: Types, Operators, and Expressions/Arithmetic Operators
 *
 */

#pragma D option quiet

BEGIN
{
	i = 0;
	i = 1 + 2 + 3;
	printf("The value of i is %d\n", i);

	i = i * 3;
	printf("The value of i is %d\n", i);

	i = (i * 3) + i;
	printf("The value of i is %d\n", i);

	i = (i + (i * 3) + i) * i;
	printf("The value of i is %d\n", i);

	i = i - (i + (i * 3) + i) * i / i * i;
	printf("The value of i is %d\n", i);

	i = i * (i - 3 + 5 / i * i ) / i * 6;
	printf("The value of i is %d\n", i);

	i = i ^ 5;
	printf("The value of i is %d\n", i);

	exit (0);
}
