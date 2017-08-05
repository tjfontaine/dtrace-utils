/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *   Test the stack action with the default stack depth.
 *
 * SECTION: Output Formatting/printf()
 *
 */

BEGIN
{
	stack();
	exit(0);
}
