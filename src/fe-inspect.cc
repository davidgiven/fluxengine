#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/fluxmapreader.h"
#include "decoders/decoders.h"
#include "protocol.h"
#include "decoders/rawbits.h"
#include "record.h"
#include "sector.h"
#include "track.h"
#include "fmt/format.h"
#include "kmedian.h"

static FlagGroup flags { &readerFlags };

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

static DoubleFlag manualClockRate(
	{ "--manual-clock-rate-us" },
	"If not zero, force this clock rate; if zero, try to autodetect it.",
	0.0);

static DoubleFlag noiseFloorFactor(
    { "--noise-floor-factor" },
    "Clock detection noise floor (min + (max-min)*factor).",
    0.01);

static DoubleFlag signalLevelFactor(
    { "--signal-level-factor" },
    "Clock detection signal level (min + (max-min)*factor).",
    0.05);

static IntFlag bands(
    { "--bands" },
    "Number of bands to use for k-median interval classification.",
    3);

void setDecoderManualClockRate(double clockrate_us)
{
    manualClockRate.setDefaultValue(clockrate_us);
}

static const std::string BLOCK_ELEMENTS[] =
{ " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█" };

/* 
* Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
static nanoseconds_t guessClock(const Fluxmap& fluxmap)
{
	if (manualClockRate != 0.0)
		return manualClockRate * 1000.0;

    uint32_t buckets[256] = {};
    FluxmapReader fr(fluxmap);

    while (!fr.eof())
    {
        unsigned interval = fr.findEvent(F_BIT_PULSE);
        if (interval > 0xff)
            continue;
        buckets[interval]++;
    }
    
    uint32_t max = *std::max_element(std::begin(buckets), std::end(buckets));
    uint32_t min = *std::min_element(std::begin(buckets), std::end(buckets));
    uint32_t noise_floor = min + (max-min)*noiseFloorFactor;
    uint32_t signal_level = min + (max-min)*signalLevelFactor;

	std::cout << "\nClock detection histogram:" << std::endl;

	bool skipping = true;
	for (int i=0; i<256; i++)
	{
		uint32_t value = buckets[i];
		if (value < noise_floor/2)
		{
			if (!skipping)
				std::cout << "..." << std::endl;
			skipping = true;
		}
		else
		{
			skipping = false;

			int bar = 320*value/max;
			int fullblocks = bar / 8;

			std::string s;
			for (int j=0; j<fullblocks; j++)
				s += BLOCK_ELEMENTS[8];
			s += BLOCK_ELEMENTS[bar & 7];

			std::cout << fmt::format("{: 3} {:.2f} {:6} {}",
					i,
					(double)i * US_PER_TICK,
					value,
					s);
			std::cout << std::endl;
		}
	}

	std::cout << fmt::format("Noise floor:  {}", noise_floor) << std::endl;
	std::cout << fmt::format("Signal level: {}", signal_level) << std::endl;

	std::vector<float> centres = optimalKMedian(fluxmap, bands);
	for (int i=0; i<bands; i++)
		std::cout << fmt::format("Band #{}:      {:.2f} us\n", i, centres[i]);

    return centres[0] * 1000.0;
}

int mainInspect(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

	const auto& tracks = readTracks();
	if (tracks.size() != 1)
		Error() << "the source dataspec must contain exactly one track (two sides count as two tracks)";

	auto& track = *tracks.begin();
	track->readFluxmap();

	std::cout << fmt::format("0x{:x} bytes of data in {:.3f}ms\n",
			track->fluxmap->bytes(),
			track->fluxmap->duration() / 1e6);
	std::cout << fmt::format("Required USB bandwidth: {}kB/s\n",
			track->fluxmap->bytes()/1024.0 / (track->fluxmap->duration() / 1e9));

	nanoseconds_t clockPeriod = guessClock(*track->fluxmap);
	std::cout << fmt::format("{:.2f} us clock detected.", (double)clockPeriod/1000.0) << std::flush;

	FluxmapReader fmr(*track->fluxmap, bands, false);
	fmr.seek(seekFlag*1000000.0);

	if (dumpFluxFlag)
	{
		std::cout << "\n\nMagnetic flux follows (times in us):" << std::endl;

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
			ticks += fmr.findEvent(F_BIT_PULSE);

			nanoseconds_t transition = ticks*NS_PER_TICK;
			nanoseconds_t next;
			
			bool clocked = false;

			bool bannered = false;
			auto banner = [&]()
			{
				std::cout << fmt::format("\n{: 10.3f}:{}", (double)next / 1000.0, clocked ? '-' : ' ');
				bannered = true;
			};

			for (;;)
			{
				next = now + resolution;
				clocked = now >= nextclock;
				if (clocked)
					nextclock += clockPeriod;
				if (next >= transition)
					break;
				banner();
				now = next;
			}

			nanoseconds_t length = transition - lasttransition;
			if (!bannered)
				banner();
			std::cout << fmt::format("==== {:06x} {: 10.3f} +{:.3f} = {:.1f} clocks",
			    fmr.tell().bytes,
				(double)transition / 1000.0,
				(double)length / 1000.0,
				(double)length / clockPeriod);
			bannered = false;
			lasttransition = transition;
		}
	}

	if (dumpBitstreamFlag)
	{
		std::cout << fmt::format("\n\nAligned bitstream from {:.3f}ms follows:\n",
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

