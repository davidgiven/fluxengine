#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "dataspec.h"
#include "protocol.h"

static FlagGroup flags;

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":d=0:t=0:s=0");

static IntFlag hardSectorCount(
    { "--hard-sector-count" },
    "number of hard sectors on the disk (0=soft sectors)",
    0);

int mainRpm(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

	Error() << "TODO";
	#if 0
    FluxSpec spec(source);
    usbSetDrive(spec.drive, false, F_INDEX_REAL);
    nanoseconds_t period = usbGetRotationalPeriod(hardSectorCount);
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
	#endif

    return 0;
}
