/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

#pragma D option strsize=1024
#pragma D option dynvarsize=512

BEGIN
{
	a["Harding"] = 1;
	a["Hoover"] = 1;
	a["Nixon"] = 1;
	a["Bush"] = 1;
}

BEGIN
{
	exit(0);
}
