#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright © 2013, 2016, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

##
#
# ASSERTION:
# Verify that we do not get aborts at destruction time due to
# races between the control thread cleanup machinery and the
# dtrace cleanup machinery, exacerbated by accidentally unlocking
# the relevant mutex at the wrong time.
#
# SECTION: dtrace Utility/-p Option
#
##

# This test, by design, takes ages.  This is about three times as
# long as it takes on my system.
# @@timeout: 1500

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1
CC=/usr/bin/gcc
CFLAGS=

DIRNAME="$tmpdir/destruction-double-unlock.$$.$RANDOM"
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
	for i in $(seq 1 1000); do
		./test &
		PID=$!
		disown %+
		if ! $dtrace -p $PID -lP test_prov$PID; then
		    kill -9 $PID
		    return 1
		fi
		kill -9 $PID
	done
}

script
status=$?

exit $status
