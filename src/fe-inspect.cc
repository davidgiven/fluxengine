#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include <fmt/format.h>

static IntFlag trackFlag(
    { "--track", "-T" },
    "Which track to inspect.",
    0);

static IntFlag sideFlag(
    { "--side", "-S" },
    "Which side to inspect.",
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

			/* For MFM, the bit clock is half the detected clock. */
			auto bitmap = decodeFluxmapToBits(fluxmap, clockPeriod/2);
			std::cout << fmt::format("{} bytes encoded.", bitmap.size()/8) << std::endl;

			for (bool bit : bitmap)
				std::cout << (bit ? 'X' : '.');
			std::cout << std::endl;
		}
    }

    return 0;
}

