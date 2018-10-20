#include "globals.h"
#include "flags.h"
#include "usb.h"

static StringFlag source(
    { "--source", "-s" },
    "source for data",
    "");

static StringFlag destination(
    { "--write-flux", "-f" },
    "write the raw magnetic flux to this file",
    "");

static SettableFlag justRead(
    { "--just-read", "-R" },
    "just read the disk but do no further processing");

int allTracks()
{
    return 0;
}

