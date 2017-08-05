/*
 * Oracle Linux DTrace.
 * Copyright © 2012, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * @@trigger: none
 */

sched:::tick
{
	trace(chip);
	n++;
}

sched:::tick
/n > 2/
{
	exit(0);
}
