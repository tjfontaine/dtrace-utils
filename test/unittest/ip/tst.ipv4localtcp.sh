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
# Test ip:::{send,receive} of IPv4 TCP to a remote host.
#
# This may fail due to:
#
# 1. A change to the ip stack breaking expected probe behavior,
#    which is the reason we are testing.
# 2. The loopback interface missing or not up.
# 3. The local ssh service is not online.
# 4. An unlikely race causes the unlocked global send/receive
#    variables to be corrupted.
#
# This test performs a TCP connection and checks that at least the
# following packet counts were traced:
#
# 3 x ip:::send (2 during the TCP handshake, then a FIN)
# 2 x ip:::receive (1 during the TCP handshake, then the FIN ACK)
#
# The actual count tested is 5 each way, since we are tracing both
# source and destination events.
#

if (( $# != 1 )); then
	echo "expected one argument: <dtrace-path>" >&2
	exit 2
fi

dtrace=$1
local=127.0.0.1
tcpport=22

cat > $tmpdir/tst.ipv4localtcp.test.pl <<-EOPERL
	use IO::Socket;
	my \$s = IO::Socket::INET->new(
	    Proto => "tcp",
	    PeerAddr => "$local",
	    PeerPort => $tcpport,
	    Timeout => 3);
	die "Could not connect to host $local port $tcpport" unless \$s;
	close \$s;
EOPERL

$dtrace $dt_flags -c '/usr/bin/perl $tmpdir/tst.ipv4localtcp.test.pl' -qs /dev/stdin <<EODTRACE
BEGIN
{
	send = receive = 0;
}

ip:::send
/args[2]->ip_saddr == "$local" && args[2]->ip_daddr == "$local" &&
    args[4]->ipv4_protocol == IPPROTO_TCP/
{
	send++;
}

ip:::receive
/args[2]->ip_saddr == "$local" && args[2]->ip_daddr == "$local" &&
    args[4]->ipv4_protocol == IPPROTO_TCP/
{
	receive++;
}

END
{
	printf("Minimum TCP events seen\n\n");
	printf("ip:::send - %s\n", send >= 5 ? "yes" : "no");
	printf("ip:::receive - %s\n", receive >= 5 ? "yes" : "no");
}
EODTRACE

status=$?

exit $status
