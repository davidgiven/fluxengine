#!/bin/sh
set -e

format="$1"
ext="$2"
fluxengine="$3"
script="$4"
flags="$5"
dir="$6"

srcfile=$dir.$format.src.img
fluxfile=$dir.$format.$ext
destfile=$dir.$format.dest.img

dd if=/dev/urandom of=$srcfile bs=1048576 count=2 2>&1

$fluxengine write $format -i $srcfile -d $fluxfile --drive.rotational_period_ms=200 $flags
$fluxengine read $format -s $fluxfile -o $destfile --drive.rotational_period_ms=200 $flags
if [ ! -s $destfile ]; then
	echo "Zero length output file!" >&2
	exit 1
fi

truncate $srcfile -r $destfile
if ! cmp $srcfile $destfile; then
	echo "Comparison failed!" >&2
	exit 1
fi
exit 0

