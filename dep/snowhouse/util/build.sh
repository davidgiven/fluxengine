#!/bin/sh

test -n "$SRCDIR" || SRCDIR="$(pwd)"
test -n "$CC" || CC=cc
test -n "$CXX" || CXX=c++

cxxstandard="$1"
case "$cxxstandard" in
11|14|17)
	;;
*)
	echo "Usage: $0 (11|14|17) [image name]" >&2
	exit 1
esac

image="$2"
test -n "$image" || image=default

buildpath="builds/$(basename "$image" | sed 's/^.*://')/$(basename "$CXX")/std$cxxstandard"

echo "Build settings:"
echo " * source directory: $SRCDIR"
echo " * C compiler: $CC"
echo " * C++ compiler: $CXX"
echo " * C++ standard: C++$cxxstandard"
echo " * image: $image"
echo " * build path: $buildpath"

mkdir -p "$SRCDIR/$buildpath"
cd "$SRCDIR/$buildpath" || exit
cmake -DSNOWHOUSE_BUILD_TESTS=1 -DSNOWHOUSE_RUN_TESTS=1 -D"SNOWHOUSE_CXX_STANDARD=C++$cxxstandard" "$SRCDIR" || exit
cmake --build . || exit
