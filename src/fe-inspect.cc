#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "decoders.h"
#include "image.h"
#include "protocol.h"
#include "rawbits.h"
#include "record.h"
#include "sector.h"
#include "track.h"
#include <fmt/format.h>

static SettableFlag dumpFluxFlag(
	{ "--dump-flux", "-F" },
	"Dump raw magnetic disk flux.");

static SettableFlag dumpBitstreamFlag(
	{ "--dump-bitstream", "-B" },
	"Dump aligned bitstream.");

static SettableFlag dumpBytecodesFlag(
    { "--dump-bytecodes", "-H" },
    "Dump the raw FluxEngine bytecodes.");

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
	track->readFluxmap();

	nanoseconds_t clockPeriod = track->fluxmap->guessClock();
	std::cout << fmt::format("       {:.2f} us clock detected; ", (double)clockPeriod/1000.0) << std::flush;

	FluxmapReader fmr(*track->fluxmap);
	fmr.seek(seekFlag*1000000.0);
	std::cout << fmt::format("{:.2f} us bit clock; ", (double)clockPeriod/1000.0) << std::flush;

	if (dumpFluxFlag)
	{
		std::cout << std::endl
					<< "Magnetic flux follows (times in us):" << std::endl;

		int resolution = fluxmapResolutionFlag;
		if (resolution == 0)
			resolution = clockPeriod / 4;

		nanoseconds_t nextclock = clockPeriod;

		nanoseconds_t now = fmr.tell().ns();
		int ticks = now / NS_PER_TICK;

		std::cout << fmt::format("{: 10.3f}:-", ticks*US_PER_TICK);
		nanoseconds_t lasttransition = 0;
		while (!fmr.eof())
		{
			ticks += fmr.readNextMatchingOpcode(F_OP_PULSE);

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
			std::cout << fmt::format("==== {:06x} {: 10.3f} +{:.3f} = {:.1f} clocks",
			    fmr.tell().bytes,
				(double)transition / 1000.0,
				(double)length / 1000.0,
				(double)length / clockPeriod);
			lasttransition = transition;
		}
	}

	if (dumpBitstreamFlag)
	{
		std::cout << fmt::format("Aligned bitstream from {:.3f}ms follows:\n",
				fmr.tell().ns() / 1000000.0);

		while (!fmr.eof())
		{
			std::cout << fmt::format("{:06x} {: 10.3f} : ",
				fmr.tell().bytes, fmr.tell().ns() / 1000000.0);
			for (unsigned i=0; i<50; i++)
			{
				if (fmr.eof())
					break;
				bool b = fmr.readRawBit(clockPeriod);
				std::cout << (b ? 'X' : '-');
			}

			std::cout << std::endl;
		}

		std::cout << std::endl;
	}

    if (dumpBytecodesFlag)
    {
        std::cout << "Raw FluxEngine bytecodes follow:" << std::endl;

        const auto& bytes = track->fluxmap->rawBytes();
        hexdump(std::cout, bytes);
    }

    return 0;
}

