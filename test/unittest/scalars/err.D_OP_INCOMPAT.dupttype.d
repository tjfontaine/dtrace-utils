/*
 * Oracle Linux DTrace.
 * Copyright © 2006, 2012, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *  Test assigning a variable two different incompatible types.
 *  Test assigning a variable two different incompatible types.
 *
 * SECTION:  Variables/Thread-Local Variables
 *
 */

BEGIN
{
	self->x = *`cad_pid;
	self->x = `max_pfn;
}
