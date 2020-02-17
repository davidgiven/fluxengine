#include "globals.h"
#include "flags.h"
#include "usb.h"

static FlagGroup flags;

int mainTestBulkTransport(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    usbTestBulkWrite();
    usbTestBulkRead();
    return 0;
}
