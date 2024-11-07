#!/bin/sh
set -e

format="$1"
ext="$2"
fluxengine="$3"
script="$4"
flags="$5"
dir="$6"

srcfile=$dir/src.img
fluxfile=$dir/flux.$ext
destfile=$dir/dest.img

dd if=/dev/urandom of=$srcfile bs=1048576 count=2 2>&1

$fluxengine write $format -i $srcfile -d $fluxfile --drive.rotational_period_ms=200 $flags
$fluxengine read $format -s $fluxfile -o $destfile --drive.rotational_period_ms=200 $flags
if [ ! -s $destfile ]; then
	echo "Zero length output file!" >&2
	exit 1
fi

truncate -r $destfile $srcfile
if ! cmp $srcfile $destfile; then
	echo "Comparison failed!" >&2
	echo "Run this to repeat:" >&2
	echo "./scripts/encodedecodetest.sh \"$1\" \"$2\" \"$3\" \"$4\" \"$5\" \"$6\"" >&2
	exit 1
fi
exit 0

