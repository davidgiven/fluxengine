#!/bin/sh
set -e

format=$1
tmp=/tmp/$$-$format
srcfile=$tmp.src.img
fluxfile=$tmp.$2
destfile=$tmp.dest.img
fluxengine=$3
shift
shift
shift

trap "rm -f $srcfile $fluxfile $destfile" EXIT

dd if=/dev/urandom of=$srcfile bs=1048576 count=2 2>&1

echo $fluxengine write $format -i $srcfile -d $fluxfile --drive.rotational_period_ms=200 "$@"
$fluxengine write $format -i $srcfile -d $fluxfile --drive.rotational_period_ms=200 "$@"
echo $fluxengine read $format -s $fluxfile -o $destfile --drive.rotational_period_ms=200 "$@"
$fluxengine read $format -s $fluxfile -o $destfile --drive.rotational_period_ms=200 "$@"
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

