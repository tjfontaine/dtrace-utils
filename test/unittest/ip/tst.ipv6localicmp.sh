#!/bin/bash
#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#

#
# Copyright 2008, 2017 Oracle, Inc.  All rights reserved.
# Use is subject to license terms.
#

#
# Test ip:::{send,receive} of IPv6 ICMP to a local address.  This creates a
# temporary lo0/inet6 interface if one doesn't already exist.
#
# This may fail due to:
#
# 1. A change to the ip stack breaking expected probe behavior,
#    which is the reason we are testing.
# 2. Unrelated ICMPv6 on lo traced by accident.
#

if (( $# != 1 )); then
	echo "expected one argument: <dtrace-path>" >&2
	exit 2
fi

dtrace=$1
local=::1

$dtrace $dt_flags -c "/bin/ping6 $local -c 3" -qs /dev/stdin <<EOF | sort -n
ip:::send
/args[2]->ip_saddr == "$local" && args[2]->ip_daddr == "$local" &&
    args[5]->ipv6_nexthdr == IPPROTO_ICMPV6/
{
	printf("1 ip:::send    (");
	printf("args[2]: %d %d, ", args[2]->ip_ver, args[2]->ip_plength);
	printf("args[5]: %d %d %d)\n",
	    args[5]->ipv6_ver, args[5]->ipv6_tclass, args[5]->ipv6_plen);
}

ip:::receive
/args[2]->ip_saddr == "$local" && args[2]->ip_daddr == "$local" &&
    args[5]->ipv6_nexthdr == IPPROTO_ICMPV6/
{
	printf("2 ip:::receive (");
	printf("args[2]: %d %d, ", args[2]->ip_ver, args[2]->ip_plength);
	printf("args[5]: %d %d %d)\n",
	    args[5]->ipv6_ver, args[5]->ipv6_tclass, args[5]->ipv6_plen);
}
EOF
