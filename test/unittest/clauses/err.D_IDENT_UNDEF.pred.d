/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION: Test a clause where an unknown identifier appears in a predicate.
 * SECTION:   Program Structure / Probe Clauses and Declarations
 *
 */

BEGIN
/x != 0/
{
	exit(0);
}
