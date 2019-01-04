#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "protocol.h"
#include <fmt/format.h>

static IntFlag trackFlag(
    { "--track", "-T" },
    "Which track to inspect.",
    0);

static IntFlag sideFlag(
    { "--side", "-S" },
    "Which side to inspect.",
    0);

static DoubleFlag clockScaleFlag(
	{ "--clock-scale" },
	"Scale the clock by this much after detection (use 0.5 for MFM, 1.0 for anything else).",
	0.5);

static BoolFlag dumpFluxFlag(
	{ "--dump-flux", "-F" },
	"Dump raw magnetic disk flux.",
	false);

static BoolFlag dumpBitstreamFlag(
	{ "--dump-bitstream", "-B" },
	"Dump aligned bitstream.",
	false);

static IntFlag fluxmapResolutionFlag(
	{ "--fluxmap-resolution" },
	"Resolution of flux visualisation (nanoseconds). 0 to autoscale",
	0);

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

    for (auto& track : readTracks())
    {
		if ((track->track == trackFlag) && (track->side == sideFlag))
		{
			Fluxmap& fluxmap = track->read();

			nanoseconds_t clockPeriod = fluxmap.guessClock();
			std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

			auto bitmap = fluxmap.decodeToBits(clockPeriod*clockScaleFlag);
			std::cout << fmt::format("{} bytes encoded.", bitmap.size()/8) << std::endl;

			if (dumpFluxFlag)
			{
				std::cout << std::endl
				          << "Magnetic flux follows (times in us):" << std::endl;

				int resolution = fluxmapResolutionFlag;
				if (resolution == 0)
					resolution = clockPeriod / 4;

				nanoseconds_t now = 0;
				nanoseconds_t nextclock = clockPeriod;
				int ticks = 0;
				std::cout << fmt::format("{: 10.3f}:-", 0.0);
				for (int cursor=0; cursor<fluxmap.bytes(); cursor++)
				{
					int interval = fluxmap[cursor];
					if (interval == 0)
						interval = 0x100;
					ticks += interval;

					nanoseconds_t transition = ticks*NS_PER_TICK;
					nanoseconds_t next;
					
					bool clocked = false;
					for (;;)
					{
						next = now + resolution;
						clocked = now >= nextclock;
						if (clocked)
							nextclock += clockPeriod;
						if (next >= transition)
							break;
						std::cout << std::endl
						          << fmt::format("{: 10.3f}:{}", (double)next / 1000.0, clocked ? '-' : ' ');
						now = next;
					}

					std::cout << fmt::format("==== {: 10.3f}", (double)transition / 1000.0);
				}
			}

			if (dumpBitstreamFlag)
			{
				std::cout << "Aligned bitstream follows:" << std::endl
						  << std::endl;

				for (bool bit : bitmap)
					std::cout << (bit ? 'X' : '.');
				std::cout << std::endl;
			}
		}
    }

    return 0;
}

