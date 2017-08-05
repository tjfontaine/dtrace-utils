#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright © 2014, 2015, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

# @@runtest-opts: -p $_pid
# @@trigger: execloop
# @@trigger-timing: after

#
# This script tests that a process execing in a tight loop can still be
# correctly grabbed.
#
if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1

$dtrace $dt_flags -n 'BEGIN { exit(0); }'
