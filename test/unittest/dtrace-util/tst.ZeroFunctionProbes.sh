#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright © 2006, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

##
#
# ASSERTION:
# The -Z option can be used to permit descriptions that match
# zero probes.
#
# SECTION: dtrace Utility/-Z Option;
# 	dtrace Utility/-f Option
#
##


if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1

$dtrace $dt_flags -qZf wassup'{printf("Iamkool");}' \
-qf read'{printf("I am done"); exit(0);}'

status=$?

if [ "$status" -ne 0 ]; then
	echo $tst: dtrace failed
fi

exit $status
