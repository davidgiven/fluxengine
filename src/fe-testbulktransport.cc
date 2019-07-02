#include "globals.h"
#include "flags.h"
#include "usb.h"

FlagGroup flags;

int main(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    usbTestBulkTransport();
    return 0;
}
