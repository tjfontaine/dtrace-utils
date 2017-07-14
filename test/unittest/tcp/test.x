#!/bin/bash

check_provider tcp
if (( $? != 0 )); then
        echo "Could not load tcp provider"
        exit 1
fi

if ! perl -MIO::Socket -e 'exit(0);' 2>/dev/null; then
	echo "No IO::Socket"
	exit 1
fi
exit 0
