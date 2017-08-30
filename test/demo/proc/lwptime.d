/*
 * Oracle Linux DTrace.
 * Copyright (c) 2005, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

proc:::lwp-start
/tid != 1/
{
	self->start = timestamp;
}

proc:::lwp-exit
/self->start/
{
	@[execname] = quantize(timestamp - self->start);
	self->start = 0;
}
