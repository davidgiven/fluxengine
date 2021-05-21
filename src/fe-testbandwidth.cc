#include "globals.h"
#include "flags.h"
#include "usb/usb.h"

static FlagGroup flags;

int mainTestBandwidth(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});
    usbTestBulkWrite();
    usbTestBulkRead();
    return 0;
}
