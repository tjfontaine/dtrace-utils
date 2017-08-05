/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION: An identifier used to alias a primitive data type can be further
 *       used to typedef other aliases. typedef is transitive.
 *
 * SECTION: Type and Constant Definitions/Typedef
 *
 * NOTES:
 *
 */

#pragma D option quiet


typedef int new_int;
typedef new_int latest_int;

BEGIN
{
	exit(0);
}
