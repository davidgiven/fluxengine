#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "dataspec.h"
#include "protocol.h"
#include "proto.h"

static FlagGroup flags;

static IntFlag driveFlag(
    { "-d", "--drive" },
    "drive to test",
	0,
	[](const auto& value)
	{
		config.mutable_input()->mutable_flux()->mutable_drive()->set_drive(value);
	});

int mainRpm(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    usbSetDrive(config.input().flux().drive().drive(), false, F_INDEX_REAL);
    nanoseconds_t period = usbGetRotationalPeriod(config.input().flux().drive().hard_sector_count());
    if (period != 0)
        std::cout << "Rotational period is " << period/1000000 << " ms (" << 60e9/period << " rpm)" << std::endl;
    else
    {
        std::cout << "No index pulses detected from the disk. Common causes of this are:\n"
                     "  - no drive is connected\n"
                     "  - the drive doesn't have an index sensor (e.g. BBC Micro drives)\n"
                     "  - the disk has no index holes (e.g. reversed flippy disks)\n"
                     "  - (most common) no disk is inserted in the drive!\n";
    }

    return 0;
}
