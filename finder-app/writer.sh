#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Error: parameters were not specified"
        exit 1
fi


writefile=$1
writestr=$2

write_dir=$(dirname "$writefile")
mkdir -p "$write_dir"

if ! echo "$writestr" > "$writefile"; then
	echo "Error: failed writing file"
	exit 1
fi

exit 0
