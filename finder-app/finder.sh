#!/bin/bash

if [ $# -ne 2 ]; then
	echo "Error: parameters were not specified"
	exit 1
fi

filesdir=$1
searchstr=$2
if [ ! -d "$filesdir" ]; then
	echo "Error: '$filesdir' is not a valid directory"
	exit 1
fi

file_cnt=$( find "$filesdir" -type f | wc -l)

match_cnt=$( grep -r "$searchstr" "$filesdir" | wc -l )

echo "The number of files are $file_cnt and the number of matching lines are $match_cnt"

exit 0
