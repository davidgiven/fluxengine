#!/bin/sh
set -e

tmp=/tmp/$$
srcfile=$tmp.src.img
fluxfile=$tmp.$2
destfile=$tmp.dest.img
format=$1
shift
shift

trap "rm -f $srcfile $fluxfile $destfile" EXIT
bs=1048576
count=2
case "${1-}" in
([0-9]*) bs=$1; count=1; shift;
esac

dd if=/dev/urandom of=$srcfile bs=$bs count=$count 2>&1

./fluxengine write $format -i $srcfile -d $fluxfile "$@"
./fluxengine read $format -s $fluxfile -o $destfile "$@"
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

