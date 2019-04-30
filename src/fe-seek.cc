#include "globals.h"
#include "flags.h"
#include "usb.h"

static IntFlag drive(
    { "--drive", "-d" },
    "drive to use",
    0);

static IntFlag track(
    { "--track", "-t" },
    "track to seek to",
    0);

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

    usbSetDrive(drive, false);
    usbSeek(track);
    return 0;
}
