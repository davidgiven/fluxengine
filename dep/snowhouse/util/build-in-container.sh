#!/bin/sh

if test "$#" -ne 2
then
	echo "Usage: $0 (11|14|17) <docker image>" >&2
	exit 2
fi

SRCDIR=/usr/src/snowhouse
docker run \
       -v "$PWD":"$SRCDIR" \
       -e "SRCDIR=$SRCDIR" \
       -e "CC=$CC" \
       -e "CXX=$CXX" \
       "$2" "$SRCDIR"/util/build.sh "$1" "$2"
