#!/bin/bash
#
# Oracle Linux DTrace.
# Copyright (c) 2007, Oracle and/or its affiliates. All rights reserved.
# Licensed under the Universal Permissive License v 1.0 as shown at
# http://oss.oracle.com/licenses/upl.
#
# This test verifies that USDT providers are removed when its associated
# load object is closed via dlclose(3dl).
#

PATH=/usr/bin:/usr/sbin:$PATH

if [ $# != 1 ]; then
	echo expected one argument: '<'dtrace-path'>'
	exit 2
fi

dtrace=$1
CC=/usr/bin/gcc
CFLAGS=
SYSCALL=pause

if [[ "$(uname -m)" = "sparc64" ]]; then
    SYSCALL='rt_sig*'
fi

DIRNAME="$tmpdir/usdt-dlclose1.$$.$RANDOM"
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

int
main(int argc, char **argv)
{
	void *live;

	if ((live = dlopen("./livelib.so", RTLD_LAZY | RTLD_LOCAL)) == NULL) {
		printf("dlopen of livelib.so failed: %s\n", dlerror());
		return (1);
	}

	(void) dlclose(live);

	pause();

	return (0);
}
EOF

make > /dev/null
if [ $? -ne 0 ]; then
	echo "failed to build" >& 2
	exit 1
fi

script() {
	$dtrace -w -x bufsize=1k -c ./main -qs /dev/stdin <<EOF
	syscall::$SYSCALL:entry
	/pid == \$target/
	{
		system("$dtrace -l -P test_prov*");
		system("kill %d", \$target);
		exit(0);
	}

	tick-1s
	/i++ == 5/
	{
		printf("failed\n");
		exit(1);
	}
EOF
}

script
status=$?

exit $status
