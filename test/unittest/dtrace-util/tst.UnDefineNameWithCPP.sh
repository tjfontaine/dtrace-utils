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
# The -D option can be used to define a name when used in conjunction
# with the -C option. The -U option can be used to undefine a name in
# conjunction with the -C option. 
#
# SECTION: dtrace Utility/-C Option;
# 	dtrace Utility/-D Option;
# 	dtrace Utility/-U Option
#
##

script()
{
	$dtrace $dt_flags -C -D VALUE=40 -U VALUE -s /dev/stdin <<EOF
	#pragma D option quiet

	BEGIN
	{
		printf("Value of VALUE: %d\n", VALUE);
		exit(0);
	}
EOF
}

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1

script
status=$?

if [ "$status" -ne 0 ]; then
	exit 0
fi

echo $tst: dtrace failed
exit $status
