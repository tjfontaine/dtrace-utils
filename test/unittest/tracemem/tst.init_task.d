/*
 * Oracle Linux DTrace.
 * Copyright © 2006, 2012, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *  Test tracemem() by tracing out the contents of the initial
 *  task_struct as a raw stream of bytes.
 *
 * SECTION: Actions and Subroutines/tracemem()
 */


BEGIN
{
	i = 1;
}

tick-1
/i != 5/
{
	tracemem((void *)&`init_task, 20);
	i++;
}

tick-1
/i == 5/
{
	exit(0);
}
