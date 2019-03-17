#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders.h"
#include "record.h"
#include "protocol.h"
#include "rawbits.h"
#include "fmt/format.h"
#include <numeric>

static DoubleFlag pulseAccuracyFactor(
    { "--pulse-accuracy-factor" },
    "Pulses must be within this much of an expected clock tick to register (in clock periods).",
    0.4);

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
    FluxmapReader fr(*this);
    while (cursor < bytes())
    {
        unsigned interval;
        int opcode = fr.readPulse(interval);
        if (opcode != 0x80)
            break;
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
        double blocks_per_count = 320.0/max;

        auto show_noise_and_signal_levels = [=] {
            /* Must be 12 chars in left margin */
            std::cout << fmt::format("        0% >{:>{}}{:>{}}{:<{}}< 100%\n",
                "|",
                int((noise_floor*blocks_per_count)/8),
                "|",
                int((signal_level - noise_floor)*blocks_per_count/8),
                "",
                int((max - signal_level)*blocks_per_count/8));
        };

        show_noise_and_signal_levels();
		bool skipping = true;
        for (int i=0; i<256; i++)
		{
			uint32_t value = buckets[i];
			if (value < noise_floor/2)
			{
				if (!skipping)
					show_noise_and_signal_levels();
				skipping = true;
			}
			else
			{
				skipping = false;

				int bar = value * blocks_per_count;
				int fullblocks = bar / 8;

				std::string s;
				for (int j=0; j<fullblocks; j++)
					s += BLOCK_ELEMENTS[8];
				s += BLOCK_ELEMENTS[bar & 7];

                /* Must be 10 chars in left margin */
				std::cout << fmt::format("{:5.2f}{:6} {}", (double)i * US_PER_TICK, value, s);
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
    nanoseconds_t pulseAccuracy = pulseAccuracyFactor * clockPeriod;

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
            int opcode = fr.read(interval);
            timestamp += interval * NS_PER_TICK;
            if (opcode == -1)
                goto abort;
            else if (opcode == 0x80)
                break;
            else if (opcode == 0x81)
                indices->push_back(count);
        }

        int clocks = (timestamp + clockPeriod/2) / clockPeriod;
        nanoseconds_t expectedClock = clocks*clockPeriod;
        if (abs(expectedClock - timestamp) > pulseAccuracy)
            continue;

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

nanoseconds_t AbstractDecoder::guessClock(Fluxmap& fluxmap, unsigned physicalTrack) const
{
    return fluxmap.guessClock();
}

RawRecordVector AbstractSoftSectorDecoder::extractRecords(const RawBits& rawbits) const
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
                    rawbits.begin() + matchStart,
                    rawbits.begin() + end)
            )
        );
    };

    while (cursor < rawbits.size())
    {
        fifo = (fifo << 1) | rawbits[cursor++];
        
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

RawRecordVector AbstractHardSectorDecoder::extractRecords(const RawBits& rawbits) const
{
    /* This is less easy than it looks.
     *
     * Hard-sectored disks contain one extra index hole, marking the top of the
     * disk. This appears halfway in between two start-of-sector index holes. We
     * need to find this and ignore it, otherwise we'll split that sector in two
     * (and it won't work).
     * 
     * The routine here is pretty simple and requires this extra hole to have
     * valid index holes either side of it --- it can't cope with the extra
     * index hole being the first or last seen. Always give it slightly more
     * than one revolution of data.
     */

    RawRecordVector records;
    const auto& indices = rawbits.indices();
    if (!indices.empty())
    {
        unsigned total = 0;
        unsigned previous = 0;
        for (unsigned index : indices)
        {
            total += index - previous;
            previous = index;
        }
        total += rawbits.size() - previous;

        unsigned sectors_must_be_bigger_than = (total / indices.size()) * 2/3;


        previous = 0;
        auto pushRecord = [&](size_t end)
        {
            if ((end - previous) < sectors_must_be_bigger_than)
                return;

            records.push_back(
                std::unique_ptr<RawRecord>(
                    new RawRecord(
                        previous,
                        rawbits.begin() + previous,
                        rawbits.begin() + end)
                )
            );
            previous = end;
        };

        for (unsigned index : indices)
            pushRecord(index);
        pushRecord(rawbits.size());
    }

    return records;
}


