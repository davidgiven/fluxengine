#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "protocol.h"

static FlagGroup flags = {
	&usbFlags,
};

static IntFlag drive(
    { "--drive", "-d" },
    "drive to use",
    0);

static IntFlag track(
    { "--track", "-t" },
    "track to seek to",
    0);

static BoolFlag doubleStep(
    { "--double-step" },
    "double-step 96tpi drives for 48tpi media",
    false);

static IntFlag stepIntervalTime(
    { "--step-interval-time" },
    "Head step interval time in milliseconds",
    6);

static IntFlag stepSettlingTime(
    { "--step-settling-time" },
    "Head step settling time in milliseconds",
    50);

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    usbSetDrive(drive, false, F_INDEX_REAL,
        stepIntervalTime, stepSettlingTime, doubleStep);
    usbSeek(track);
    return 0;
}
