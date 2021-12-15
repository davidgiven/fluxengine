# This script converts libusbp into a format that can easily be dropped into
# other projects: one source file, one C header file, and one C++ header file.
#
# This is handy for people who want to use libusbp in their own C/C++ software
# projects but don't want to deal with the issues that come with having one more
# dependency.
#
# It also compiles the lsusb example program using the drop-in library.

set -eou pipefail

cd `dirname $0`
SRC=../../src

{
  echo "#if 0"
  cat $SRC/../LICENSE.txt
  echo "#endif"
  cat $SRC/libusbp_internal.h
  cat $SRC/*.c
  echo "#ifdef _WIN32"
  cat $SRC/windows/*.c
  echo "#endif"
  echo "#ifdef __linux__"
  cat $SRC/linux/*.c
  echo "#endif"
  echo "#ifdef __APPLE__"
  cat $SRC/mac/*.c
  echo "#endif"
} | sed 's/#include <libusbp_internal.h>//' > libusbp.c

cp ../lsusb/lsusb.cpp $SRC/../include/libusbp.h* .

# TODO: fix the library so we don't need -fpermissive here

g++ -std=gnu++11 -Wall -fpermissive -g -O2 -I. \
    -DLIBUSBP_DROP_IN -DLIBUSBP_STATIC \
    lsusb.cpp libusbp.c -ludev -o lsusb
