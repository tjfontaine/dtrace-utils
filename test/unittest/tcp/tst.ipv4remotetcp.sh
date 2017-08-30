#!/bin/bash

#
# Oracle Linux DTrace.
# Copyright (c) 2017, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.

#
# Test tcp:::{send,receive} of IPv4 TCP to a remote host.
#
# This may fail due to:
#
# 1. A change to the tcp stack breaking expected probe behavior,
#    which is the reason we are testing.
# 2. No physical network interface is plumbed and up.
# 3. No other hosts on this subnet are reachable and listening on ssh.
# 4. An unlikely race causes the unlocked global send/receive
#    variables to be corrupted.
#
# This test performs a TCP connection and checks that at least the
# following packet counts were traced:
#
# 3 x tcp:::send (2 during the TCP handshake, then a FIN)
# 2 x tcp:::receive (1 during the TCP handshake, then the FIN ACK)
#

if (( $# != 1 )); then
	echo "expected one argument: <dtrace-path>" >&2
	exit 2
fi

dtrace=$1
testdir="$(dirname $_test)"
getaddr=$testdir/../ip/get.ipv4remote.pl
tcpports="22 80"
tcpport=""
dest=""

if [[ ! -x $getaddr ]]; then
        echo "could not find or execute sub program: $getaddr" >&2
        exit 3
fi

for port in $tcpports ; do
	res=`$getaddr $port 2>/dev/null`
	if (( $? == 0 )); then
		read s d <<< $res
		tcpport=$port
		source=$s
		dest=$d
		break
	fi
done

if [[ -z $tcpport ]]; then
	exit 67
fi

cd $tmpdir

cat > test.pl <<-EOPERL
	use IO::Socket;
	my \$s = IO::Socket::INET->new(
	    Proto => "tcp",
	    PeerAddr => "$dest",
	    PeerPort => $tcpport,
	    Timeout => 3);
	die "Could not connect to host $dest port $tcpport" unless \$s;
	close \$s;
EOPERL

$dtrace $dt_flags -c '/usr/bin/perl test.pl' -qs /dev/stdin <<EODTRACE
BEGIN
{
	send = receive = 0;
}

tcp:::send
/args[2]->ip_saddr == "$source" && args[2]->ip_daddr == "$dest" /
{
	send++;
}

tcp:::receive
/args[2]->ip_saddr == "$dest" && args[2]->ip_daddr == "$source" /
{
	receive++;
}

END
{
	printf("Minimum TCP events seen\n\n");
	printf("tcp:::send - %s\n", send >= 2 ? "yes" : "no");
	printf("tcp:::receive - %s\n", receive >= 1 ? "yes" : "no");
}
EODTRACE
