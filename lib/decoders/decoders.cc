#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders.h"
#include "protocol.h"
#include "fmt/format.h"

static DoubleFlag clockDecodeThreshold(
    { "--clock-decode-threshold" },
    "Pulses below this fraction of a clock tick are considered spurious and ignored.",
    0.80);

static SettableFlag showClockHistogram(
    { "--show-clock-histogram" },
    "Dump the clock detection histogram.");

/* 
 * Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
nanoseconds_t Fluxmap::guessClock() const
{
    uint32_t buckets[256] = {};
    for (uint8_t interval : _intervals)
        buckets[interval]++;
    
    uint32_t max = *std::max_element(std::begin(buckets), std::end(buckets));
    uint32_t min = *std::min_element(std::begin(buckets), std::end(buckets));
    uint32_t noise_floor = (min+max)/100;
    uint32_t signal_level = noise_floor * 5;

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
        for (int i=0; i<256; i++)
            std::cout << fmt::format("{:.2f} {}", (double)i * US_PER_TICK, buckets[i]) << std::endl;

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

