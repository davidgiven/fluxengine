#include "globals.h"
#include "flags.h"
#include "usb.h"

static IntFlag track(
    { "--track", "-t" },
    "track to seek to",
    0);

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

    usbSeek(track);
    return 0;
}
