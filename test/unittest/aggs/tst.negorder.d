/*
 * Oracle Linux DTrace.
 * Copyright (c) 2006, Oracle and/or its affiliates. All rights reserved.
 * Licensed under the Universal Permissive License v 1.0 as shown at
 * http://oss.oracle.com/licenses/upl.
 */

#pragma D option quiet

BEGIN
{
	a = 7;
	b = 13;
	val = (-a * b) + a;
}

tick-1ms
{
	incr = val % b;
	val += a;
}

tick-1ms
/val == 0/
{
	val += a;
}

tick-1ms
/incr != 0/
{
	i++;
	@quanty[i] = quantize(1, incr);
	@lquanty[i] = lquantize(1, -10, 10, 1, incr);
	@summy[i] = sum(incr);
	@maxxy[i] = max(incr);
	@minny[i] = min(incr);
}

tick-1ms
/incr == 0/
{
	printf("Ordering of quantize() with some negative weights:\n");
	printa(@quanty);
	printf("\n");

	printf("Ordering of lquantize() with some negative weights:\n");
	printa(@lquanty);
	printf("\n");

	printf("Ordering of sum() with some negative weights:\n");
	printa(@summy);
	printf("\n");

	printf("Ordering of max() with some negative weights:\n");
	printa(@maxxy);
	printf("\n");

	printf("Ordering of min() with some negative weights:\n");
	printa(@minny);
	printf("\n");

	exit(0);
}
