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

static DoubleFlag clockDecodeThreshold(
    { "--clock-decode-threshold" },
    "Pulses below this fraction of a clock tick are considered spurious and ignored.",
    0.80);

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

/* Decodes a fluxmap into a nice aligned array of bits. */
const RawBits Fluxmap::decodeToBits(nanoseconds_t clockPeriod) const
{
    int pulses = duration() / clockPeriod;
    nanoseconds_t lowerThreshold = clockPeriod * clockDecodeThreshold;

    auto bitmap = std::make_unique<std::vector<bool>>(pulses);
    auto indices = std::make_unique<std::vector<size_t>>();

    unsigned count = 0;
    nanoseconds_t timestamp = 0;
    FluxmapReader fr(*this);
    for (;;)
    {
        for (;;)
        {
            unsigned interval;
            int opcode = fr.readOpcode(interval);
            timestamp += interval * NS_PER_TICK;
            if (opcode == -1)
                goto abort;
            else if ((opcode == 0x80) && (timestamp >= lowerThreshold))
                break;
            else if (opcode == 0x81)
                indices->push_back(count);
        }

        int clocks = (timestamp + clockPeriod/2) / clockPeriod;
        count += clocks;
        if (count >= bitmap->size())
            goto abort;
        bitmap->at(count) = true;
        timestamp = 0;
    }
abort:

    RawBits rawbits(std::move(bitmap), std::move(indices));
    return rawbits;
}

nanoseconds_t AbstractDecoder::guessClock(Track& track) const
{
	if (manualClockRate != 0.0)
		return manualClockRate * 1000.0;
    return guessClockImpl(track);
}

nanoseconds_t AbstractDecoder::guessClockImpl(Track& track) const
{
    return track.fluxmap->guessClock();
}

void AbstractStatefulDecoder::decodeToSectors(Track& track)
{
    Sector sector;
    sector.physicalSide = track.physicalSide;
    sector.physicalTrack = track.physicalTrack;

    FluxmapReader fmr(*track.fluxmap);
    for (;;)
    {
        nanoseconds_t clockPeriod = findSector(fmr, track);
        if (fmr.eof() || !clockPeriod)
            break;

        sector.status = Sector::MISSING;
        sector.data.clear();
        sector.clock = clockPeriod;
        _recordStart = sector.position = fmr.tell();

        decodeSingleSector(fmr, track, sector);
        pushRecord(fmr, track, sector);
        if (sector.status != Sector::MISSING)
            track.sectors.push_back(sector);
    }
}

void AbstractStatefulDecoder::pushRecord(FluxmapReader& fmr, Track& track, Sector& sector)
{
    RawRecord record;
    record.physicalSide = track.physicalSide;
    record.physicalTrack = track.physicalTrack;
    record.clock = sector.clock;
    record.position = _recordStart;

    Fluxmap::Position here = fmr.tell();
    fmr.seek(_recordStart);
    record.data = toBytes(fmr.readRawBits(here, sector.clock));
    track.rawrecords.push_back(record);
    _recordStart = here;
}

void AbstractStatefulDecoder::discardRecord(FluxmapReader& fmr)
{
    _recordStart = fmr.tell();
}

void AbstractSplitDecoder::decodeSingleSector(FluxmapReader& fmr, Track& track, Sector& sector)
{
    decodeHeader(fmr, track, sector);
    if (sector.status == Sector::MISSING)
        return;
    pushRecord(fmr, track, sector);

    nanoseconds_t clockPeriod = findData(fmr, track);
    if (fmr.eof() || !clockPeriod)
        return;
    sector.clock = clockPeriod;

    discardRecord(fmr);
    Fluxmap::Position pos = fmr.tell();
    decodeData(fmr, track, sector);
    if (sector.status == Sector::DATA_MISSING)
    {
        fmr.seek(pos);
        return;
    }
}
