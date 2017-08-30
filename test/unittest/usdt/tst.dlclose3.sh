#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright (c) 2007, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#
# @@skip: pid provider not yet implemented

#
# This test verifies that performing a dlclose(3dl) on a library doesn't
# cause existing pid provider probes to become invalid.
#

PATH=/usr/bin:/usr/sbin:$PATH

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1
CC=/usr/bin/gcc
CFLAGS=

DIRNAME="$tmpdir/usdt-dlclose3.$$.$RANDOM"
mkdir -p $DIRNAME
cd $DIRNAME

cat > Makefile <<EOF
all: main livelib.so deadlib.so

main: main.o prov.o
	\$(CC) -o main main.o -ldl

main.o: main.c
	\$(CC) -c main.c

livelib.so: livelib.o prov.o
	\$(CC) -shared -o livelib.so livelib.o prov.o -lc

livelib.o: livelib.c prov.h
	\$(CC) -c livelib.c

prov.o: livelib.o prov.d
	$dtrace -G -s prov.d livelib.o

prov.h: prov.d
	$dtrace -h -s prov.d

deadlib.so: deadlib.o
	\$(CC) -shared -o deadlib.so deadlib.o -lc

deadlib.o: deadlib.c
	\$(CC) -c deadlib.c

clean:
	rm -f main.o livelib.o prov.o prov.h deadlib.o

clobber: clean
	rm -f main livelib.so deadlib.so
EOF

cat > prov.d <<EOF
provider test_prov {
	probe go();
};
EOF

cat > livelib.c <<EOF
#include "prov.h"

void
go(void)
{
	TEST_PROV_GO();
}
EOF

cat > deadlib.c <<EOF
void
go(void)
{
}
EOF


cat > main.c <<EOF
#include <dlfcn.h>
#include <unistd.h>
#include <stdio.h>

static void
foo(void)
{
	(void) close(-1);
}

int
main(int argc, char **argv)
{
	void *live;

	if ((live = dlopen("./livelib.so", RTLD_LAZY | RTLD_LOCAL)) == NULL) {
		printf("dlopen of livelib.so failed: %s\n", dlerror());
		return (1);
	}

	(void) dlclose(live);

	foo();

	return (0);
}
EOF

make > /dev/null
if [ $? -ne 0 ]; then
	echo "failed to build" >& 2
	exit 1
fi

script() {
	$dtrace -c ./main -s /dev/stdin <<EOF
	pid\$target:a.out:foo:entry
	{
		gotit = 1;
		exit(0);
	}

	tick-1s
	/i++ == 5/
	{
		printf("test timed out");
		exit(1);
	}

	END
	/!gotit/
	{
		printf("program ended without hitting probe");
		exit(1);
	}
EOF
}

script
status=$?

exit $status
