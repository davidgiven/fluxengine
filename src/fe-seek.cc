#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "flaggroups/fluxsourcesink.h"
#include "protocol.h"

static FlagGroup flags = {
	&fluxSourceSinkFlags,
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

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    usbSetDrive(drive, false, F_INDEX_REAL);
	if (fluxSourceSinkFortyTrack)
	{
		if (track & 1)
			Error() << "you can only seek to even tracks on a 40-track drive";
		usbSeek(track / 2);
	}
	else
		usbSeek(track);
    return 0;
}
