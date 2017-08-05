/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 * 	Basic test - call with 1
 *
 * SECTION: Actions and Subroutines/exit()
 */

#pragma D option quiet

dtrace:::BEGIN
{
	exit(1);
}
