/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 * Declare a self value and assign value of 'this' variable to that.
 *
 * SECTION:  Variables/Scalar Variables
 *
 */
self int x;
this int y;

BEGIN
{
	self->x = 123;
	this->y = self->x;
	exit (0);
}
