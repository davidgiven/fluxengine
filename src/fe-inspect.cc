#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "protocol.h"
#include <fmt/format.h>

static DoubleFlag clockScaleFlag(
	{ "--clock-scale" },
	"Scale the clock by this much after detection (use 0.5 for MFM, 1.0 for anything else).",
	0.5);

static SettableFlag dumpFluxFlag(
	{ "--dump-flux", "-F" },
	"Dump raw magnetic disk flux.");

static SettableFlag dumpBitstreamFlag(
	{ "--dump-bitstream", "-B" },
	"Dump aligned bitstream.");

static IntFlag fluxmapResolutionFlag(
	{ "--fluxmap-resolution" },
	"Resolution of flux visualisation (nanoseconds). 0 to autoscale",
	0);

static DoubleFlag seekFlag(
	{ "--seek", "-S" },
	"Seek this many milliseconds into the track before displaying it.",
	0.0);

int main(int argc, const char* argv[])
{
    Flag::parseFlags(argc, argv);

	const auto& tracks = readTracks();
	if (tracks.size() != 1)
		Error() << "the source dataspec must contain exactly one track (two sides count as two tracks)";

	auto& track = *tracks.begin();
	std::unique_ptr<Fluxmap> fluxmap = track->read();

	nanoseconds_t clockPeriod = fluxmap->guessClock();
	std::cout << fmt::format("       {:.2f} us clock detected; ", (double)clockPeriod/1000.0) << std::flush;

	clockPeriod *= clockScaleFlag;
	std::cout << fmt::format("{:.2f} us bit clock; ", (double)clockPeriod/1000.0) << std::flush;

	auto bitmap = fluxmap->decodeToBits(clockPeriod);
	std::cout << fmt::format("{} bytes encoded.", bitmap.size()/8) << std::endl;

	if (dumpFluxFlag)
	{
		std::cout << std::endl
					<< "Magnetic flux follows (times in us):" << std::endl;

		int resolution = fluxmapResolutionFlag;
		if (resolution == 0)
			resolution = clockPeriod / 4;

		nanoseconds_t nextclock = clockPeriod;

		nanoseconds_t now = 0;
		int cursor = 0;
		nanoseconds_t seekto = seekFlag*1000000.0;
		int ticks = 0;
		while (cursor < fluxmap->bytes())
		{
			int interval = (*fluxmap)[cursor++];
			if (interval == 0)
				interval = 0x100;
			ticks += interval;
			
			now = ticks * NS_PER_TICK;
			if (now >= seekto)
				break;
		}

		std::cout << fmt::format("{: 10.3f}:-", ticks*US_PER_TICK);
		nanoseconds_t lasttransition = 0;
		while (cursor < fluxmap->bytes())
		{
			int interval = (*fluxmap)[cursor++];
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

			nanoseconds_t length = transition - lasttransition;
			std::cout << fmt::format("==== {: 10.3f} +{:.3f} = {:.1f} clocks",
				(double)transition / 1000.0,
				(double)length / 1000.0,
				(double)length / clockPeriod);
			lasttransition = transition;
		}
	}

	if (dumpBitstreamFlag)
	{
		std::cout << "Aligned bitstream of length " << bitmap.size()
					<< " follows:" << std::endl
					<< std::endl;

		size_t cursor = seekFlag*1000000.0 / clockPeriod;
		while (cursor < bitmap.size())
		{
			std::cout << (bitmap[cursor] ? 'X' : '-');
			cursor++;
		}

		std::cout << std::endl;
	}

    return 0;
}

