/*
 * Oracle Linux DTrace.
 * Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

/*
 * ASSERTION: Bit-fields must be positive and non-zero.
 *
 * SECTION: Structs and Unions/Bit-Fields
 */

#pragma D option quiet

struct bitRecord{
	int a : 1;
	int b : 0;
	int c : 12;
} var;

BEGIN
{
	exit(0);
}
