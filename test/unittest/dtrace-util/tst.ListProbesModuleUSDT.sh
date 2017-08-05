#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright © 2013, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

##
#
# ASSERTION:
# Testing -lm option with USDT probes with a valid module name,
# both with and without wildcarding.
#
# SECTION: dtrace Utility/-lm Option
#
##

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1
CC=/usr/bin/gcc
CFLAGS=

DIRNAME="$tmpdir/list-probes-module-usdt.$$.$RANDOM"
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

int
main(int argc, char **argv)
{
	TEST_PROV_GO();
	sleep(10);
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

script()
{
	$dtrace -c ./test -lm test
	$dtrace -c ./test -lm 'tes*'
	./test &
	PID=$!
	disown %+
	$dtrace -p $PID -lm test
	$dtrace -p $PID -lm 'tes*'
	kill -9 $PID
}

script
status=$?

exit $status
