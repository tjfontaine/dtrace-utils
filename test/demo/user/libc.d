/*
 * Oracle Linux DTrace.
 * Copyright © 2005, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/* @@runtest-opts: -c /bin/date */
/* @@xfail: userspace probing not yet implemented */

pid$target:libc.so::entry
{
	@[probefunc] = count();
}
