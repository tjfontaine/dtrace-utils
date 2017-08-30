#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright (c) 2007, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
# @@xfail: Linux ld does not seem to supoprt STV_ELIMINATE
#
# Make sure temporary symbols generated due to DTrace probes in static
# functions are removed in the final link step.
#

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1
CC=/usr/bin/gcc
CFLAGS=

DIRNAME="$tmpdir/usdt-eliminate.$$.$RANDOM"
mkdir -p $DIRNAME
cd $DIRNAME

cat > prov.d <<EOF
provider test_prov {
	probe go();
};
EOF

$dtrace -h -s prov.d
if [ $? -ne 0 ]; then
	echo "failed to generate header file" >& 2
	exit 1
fi

cat > test.c <<EOF
#include <sys/types.h>
#include "prov.h"

static void
foo(void)
{
	TEST_PROV_GO();
}

int
main(int argc, char **argv)
{
	foo();

	return (0);
}
EOF

${CC} ${CFLAGS} -c test.c
if [ $? -ne 0 ]; then
	echo "failed to compile test.c" >& 2
	exit 1
fi
$dtrace -G -s prov.d test.o
if [ $? -ne 0 ]; then
	echo "failed to create DOF" >& 2
	exit 1
fi
${CC} ${CFLAGS} -o test test.o prov.o
if [ $? -ne 0 ]; then
	echo "failed to link final executable" >& 2
	exit 1
fi

nm test.o | grep \$dtrace > /dev/null
if [ $? -ne 0 ]; then
	echo "no temporary symbols in the object file" >& 2
	exit 1
fi

nm test | grep \$dtrace > /dev/null
if [ $? -eq 0 ]; then
	echo "failed to eliminate temporary symbols" >& 2
	exit 1
fi

exit 0
