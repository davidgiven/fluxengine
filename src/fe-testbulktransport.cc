#include "globals.h"
#include "flags.h"
#include "usb.h"

static FlagGroup flags;

int mainTestBulkTransport(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    usbTestBulkTransport();
    return 0;
}
