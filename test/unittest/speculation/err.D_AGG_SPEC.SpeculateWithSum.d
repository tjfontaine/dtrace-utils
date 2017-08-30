/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 * Aggregating functions may never be speculative.
 *
 * SECTION: Speculative Tracing/Using a Speculation
 *
 */
#pragma D option quiet

BEGIN
{
	i = 0;
}

profile:::tick-1sec
/i < 1/
{
	var = speculation();
	speculate(var);
	printf("Speculation ID: %d", var);
	@sums["speculate"] = sum(i);
	i++;
}

profile:::tick-1sec
/1 == i/
{
	exit(0);
}

ERROR
{
	exit(0);
}
