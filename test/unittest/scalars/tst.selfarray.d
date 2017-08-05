/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *
 * Declare a self variable and assign appropriate value.
 *
 * SECTION:  Variables/Scalar Variables
 *
 */

self int y;
self int z;
self int res;

BEGIN
{
	self->x[self->y, self->z] = 123;
	self->res = self->x[self->y, self->z]++;
	printf("The result = %d\n", self->res);
	exit (0);
}
