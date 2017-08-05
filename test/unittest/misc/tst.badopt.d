/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

#pragma D option quiet

BEGIN
{
	setopt("Nixon");
	setopt("Harding");
	setopt("Hoover");
	setopt("Bush");
	setopt("quiet", "hell no");
	setopt("aggrate", "0.5hz");
	setopt("bufsize", "1m");
	exit(0);
}
