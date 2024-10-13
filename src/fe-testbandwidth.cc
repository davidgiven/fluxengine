#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/usb/usb.h"

static FlagGroup flags;

int mainTestBandwidth(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});
    usbTestBulkWrite();
    usbTestBulkRead();
    return 0;
}
