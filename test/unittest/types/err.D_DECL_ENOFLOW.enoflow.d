/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION:
 *	Define an enumerations with a overflow value.
 *
 * SECTION: Type and Constant Definitions/Enumerations
 */

enum e {
	toobig = 2147483648
};
