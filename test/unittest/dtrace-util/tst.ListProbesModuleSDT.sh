#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#

##
#
# ASSERTION:
# Testing -lm option with SDT probes in an existing kernel module,
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

$dtrace -lm dt_test && $dtrace -lm 'dt_t*'
status=$?

exit $status
