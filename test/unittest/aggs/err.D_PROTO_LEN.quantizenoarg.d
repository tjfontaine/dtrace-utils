/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *  quantize() should not accept a call with no arguments
 *
 * SECTION: Aggregations/Aggregations
 *
 */

BEGIN
{
	@a[1] = quantize();
}

