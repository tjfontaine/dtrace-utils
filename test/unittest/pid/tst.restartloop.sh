#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright (c) 2014, 2017, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

# @@runtest-opts: -p $_pid
# @@trigger: execloop
# @@trigger-timing: after
# @@skip: until the pid provider is implemented

#
# This script tests that a process execing in a tight loop can still be
# correctly grabbed.
#
script()
{
	# Just quit at once.
	$dtrace -Z $dt_flags -s /dev/stdin <<EOF
	BEGIN
	{
		exit(0);
	}

        pid\$1:libnonexistent.so.1::entry
        {
        }
EOF
}

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1

NUM=0;
while [[ $NUM -lt 5000 ]]; do
	NUM=$((NUM+1))
	if ! script; then
		exit 1
	fi
done
