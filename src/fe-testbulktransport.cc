#include "globals.h"
#include "flags.h"
#include "usb.h"

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);
    usbTestBulkTransport();
    return 0;
}
