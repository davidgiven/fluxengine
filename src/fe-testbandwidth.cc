#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/usb/usb.h"

static FlagGroup flags;

int mainTestBandwidth(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});
    auto usb = USB::create();
    usbTestBulkWrite();
    usbTestBulkRead();
    return 0;
}
