#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "writer.h"
#include "protocol.h"
#include "fmt/format.h"
#include <fstream>
#include <ctype.h>

static FlagGroup flags { &writerFlags };

static DoubleFlag interval(
	{ "--interval" },
	"Interval between pulses (microseconds).",
	4.0);

static DoubleFlag sequenceLength(
	{ "--sequence-length" },
	"Total length of test pattern (milliseconds).",
	200.0);

int mainWriteTestPattern(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    unsigned ticksPerInterval = (unsigned) (interval * TICKS_PER_US);

	Error() << "TODO";
	#if 0
    writeTracks(
        [&](int physicalTrack, int physicalSide) -> std::unique_ptr<Fluxmap>
        {
			std::unique_ptr<Fluxmap> fluxmap(new Fluxmap);

            while (fluxmap->duration() < (sequenceLength*1000000.0))
            {
                fluxmap->appendInterval(ticksPerInterval);
                fluxmap->appendPulse();
            }

			return fluxmap;
        }
    );
	#endif

    return 0;
}

