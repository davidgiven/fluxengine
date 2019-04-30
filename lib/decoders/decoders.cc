#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "decoders.h"
#include "record.h"
#include "protocol.h"
#include "rawbits.h"
#include "track.h"
#include "sector.h"
#include "fmt/format.h"
#include <numeric>

static SettableFlag showClockHistogram(
    { "--show-clock-histogram" },
    "Dump the clock detection histogram.");

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

void setDecoderManualClockRate(double clockrate_us)
{
    manualClockRate.value = clockrate_us;
}

static const std::string BLOCK_ELEMENTS[] =
{ " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█" };

/* 
* Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
nanoseconds_t Fluxmap::guessClock() const
{
	if (manualClockRate != 0.0)
		return manualClockRate * 1000.0;

    uint32_t buckets[256] = {};
    FluxmapReader fr(*this);

    while (!fr.eof())
    {
        unsigned interval = fr.readNextMatchingOpcode(F_OP_PULSE);
        if (interval > 0xff)
            continue;
        buckets[interval]++;
    }
    
    uint32_t max = *std::max_element(std::begin(buckets), std::end(buckets));
    uint32_t min = *std::min_element(std::begin(buckets), std::end(buckets));
    uint32_t noise_floor = min + (max-min)*noiseFloorFactor;
    uint32_t signal_level = min + (max-min)*signalLevelFactor;

    /* Find a point solidly within the first pulse. */

    int pulseindex = 0;
    while (pulseindex < 256)
    {
        if (buckets[pulseindex] > signal_level)
            break;
        pulseindex++;
    }
    if (pulseindex == -1)
        return 0;

    /* Find the upper and lower bounds of the pulse. */

    int peaklo = pulseindex;
    while (peaklo > 0)
    {
        if (buckets[peaklo] < noise_floor)
            break;
        peaklo--;
    }

    int peakhi = pulseindex;
    while (peakhi < 255)
    {
        if (buckets[peakhi] < noise_floor)
            break;
        peakhi++;
    }

    /* Find the total accumulated size of the pulse. */

    uint32_t total_size = 0;
    for (int i = peaklo; i < peakhi; i++)
        total_size += buckets[i];

    /* Now find the median. */

    uint32_t count = 0;
    int median = peaklo;
    while (median < peakhi)
    {
        count += buckets[median];
        if (count > (total_size/2))
            break;
        median++;
    }

    if (showClockHistogram)
    {
        std::cout << "Clock detection histogram:" << std::endl;

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

				std::cout << fmt::format("{:.2f} {:6} {}", (double)i * US_PER_TICK, value, s);
				std::cout << std::endl;
			}
		}

        std::cout << fmt::format("Noise floor:  {}", noise_floor) << std::endl;
        std::cout << fmt::format("Signal level: {}", signal_level) << std::endl;
        std::cout << fmt::format("Peak start:   {} ({:.2f} us)", peaklo, peaklo*US_PER_TICK) << std::endl;
        std::cout << fmt::format("Peak end:     {} ({:.2f} us)", peakhi, peakhi*US_PER_TICK) << std::endl;
        std::cout << fmt::format("Median:       {} ({:.2f} us)", median, median*US_PER_TICK) << std::endl;
    }

    /* 
     * Okay, the median should now be a good candidate for the (or a) clock.
     * How this maps onto the actual clock rate depends on the encoding.
     */

    return median * NS_PER_TICK;
}

void AbstractDecoder::decodeToSectors(Track& track)
{
    Sector sector;
    sector.physicalSide = track.physicalSide;
    sector.physicalTrack = track.physicalTrack;
    FluxmapReader fmr(*track.fluxmap);

    _track = &track;
    _sector = &sector;
    _fmr = &fmr;

    for (;;)
    {
        Fluxmap::Position recordStart = sector.position = fmr.tell();
        sector.clock = 0;
        sector.status = Sector::MISSING;
        sector.data.clear();
        sector.logicalSector = sector.logicalSide = sector.logicalTrack = 0;
        RecordType r = advanceToNextRecord();
        if (fmr.eof() || !sector.clock)
            return;
        if ((r == UNKNOWN_RECORD) || (r == DATA_RECORD))
        {
            fmr.readNextMatchingOpcode(F_OP_PULSE);
            continue;
        }

        /* Read the sector record. */

        recordStart = fmr.tell();
        decodeSectorRecord();
        pushRecord(recordStart, fmr.tell());
        if (sector.status == Sector::DATA_MISSING)
        {
            /* The data is in a separate record. */

            r = advanceToNextRecord();
            if (r == DATA_RECORD)
            {
                recordStart = fmr.tell();
                decodeDataRecord();
                pushRecord(recordStart, fmr.tell());
            }
        }

        if (sector.status != Sector::MISSING)
            track.sectors.push_back(sector);
    }
}

void AbstractDecoder::pushRecord(const Fluxmap::Position& start, const Fluxmap::Position& end)
{
    Fluxmap::Position here = _fmr->tell();

    RawRecord record;
    record.physicalSide = _track->physicalSide;
    record.physicalTrack = _track->physicalTrack;
    record.clock = _sector->clock;
    record.position = start;

    _fmr->seek(start);
    record.data = toBytes(_fmr->readRawBits(end, _sector->clock));
    _track->rawrecords.push_back(record);
    _fmr->seek(here);
}
