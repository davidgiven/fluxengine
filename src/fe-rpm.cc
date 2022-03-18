#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "protocol.h"
#include "proto.h"

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "-s", "--source" },
	"'drive:' flux source to use",
	"",
	[](const auto& value)
	{
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

int mainRpm(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

	if (!config.flux_source().has_drive())
		Error() << "this only makes sense with a real disk drive";

    usbSetDrive(config.drive().drive(), false, config.drive().index_mode());
    nanoseconds_t period = usbGetRotationalPeriod(config.drive().hard_sector_count());
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
