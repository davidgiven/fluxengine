#!/bin/sh
set -e
tmp=/tmp/$$
srcfile=$tmp.src.img
fluxfile=$tmp.flux
destfile=$tmp.dest.img
format=$1

trap "rm -f $srcfile $fluxfile $destfile" EXIT

dd if=/dev/urandom of=$srcfile bs=1M count=2

./fluxengine write $format -i $srcfile -d $fluxfile
./fluxengine read $format -s $fluxfile -o $destfile
if [ ! -s $destfile ]; then
	echo "Zero length output file!"
	exit 1
fi

truncate $srcfile -r $destfile
if ! cmp $srcfile $destfile; then
	echo "Comparison failed!"
	exit 1
fi
exit 0

