#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders.h"
#include "record.h"
#include "protocol.h"
#include "fmt/format.h"

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
    size_t cursor = 0;
    while (cursor < bytes())
    {
        uint32_t interval = getAndIncrement(cursor);
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
std::vector<bool> Fluxmap::decodeToBits(nanoseconds_t clockPeriod) const
{
    int pulses = duration() / clockPeriod;
    nanoseconds_t lowerThreshold = clockPeriod * clockDecodeThreshold;

    std::vector<bool> bitmap(pulses);
    unsigned count = 0;
    size_t cursor = 0;
    nanoseconds_t timestamp = 0;
    for (;;)
    {
        while (timestamp < lowerThreshold)
        {
            if (cursor >= bytes())
                goto abort;
            uint8_t interval = getAndIncrement(cursor);
            timestamp += interval * NS_PER_TICK;
        }

        int clocks = (timestamp + clockPeriod/2) / clockPeriod;
        count += clocks;
        if (count >= bitmap.size())
            goto abort;
        bitmap[count] = true;
        timestamp = 0;
    }
abort:

    return bitmap;
}

nanoseconds_t AbstractDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock();
}

RawRecordVector AbstractSoftSectorDecoder::extractRecords(std::vector<bool> bits) const
{
    RawRecordVector records;
    uint64_t fifo = 0;
    size_t cursor = 0;
    int matchStart = -1;

    auto pushRecord = [&](size_t end)
    {
        if (matchStart == -1)
            return;

        records.push_back(
            std::unique_ptr<RawRecord>(
                new RawRecord(
                    matchStart,
                    bits.begin() + matchStart,
                    bits.begin() + end)
            )
        );
    };

    while (cursor < bits.size())
    {
        fifo = (fifo << 1) | bits[cursor++];
        
        int match = recordMatcher(fifo);
        if (match > 0)
        {
            pushRecord(cursor - match);
            matchStart = cursor - match;
        }
    }
    pushRecord(cursor);
    
    return records;
}

