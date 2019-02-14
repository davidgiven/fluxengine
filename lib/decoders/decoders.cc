#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "decoders.h"
#include "protocol.h"
#include "fmt/format.h"

static IntFlag clockDetectionNoiseFloor(
    { "--clock-detection-noise-floor" },
    "Noise floor used for clock detection in flux.",
    200);

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
    
    int peaklo = 1;
    while (peaklo < 256)
    {
        if (buckets[peaklo] > (uint32_t)(clockDetectionNoiseFloor*2))
            break;
        peaklo++;
    }

    uint32_t peakmaxindex = peaklo;
    uint32_t peakmaxvalue = buckets[peakmaxindex];
    uint32_t peakhi = peaklo;
    while (peakhi < 256)
    {
        uint32_t v = buckets[peakhi];
        if (buckets[peakhi] < (uint32_t)clockDetectionNoiseFloor)
            break;
        if (v > peakmaxvalue)
        {
            peakmaxindex = peakhi;
            peakmaxvalue = v;
        }
        peakhi++;
    }

    if (showClockHistogram)
    {
        std::cout << "Clock detection histogram:" << std::endl;
        for (int i=0; i<256; i++)
        {
            std::cout << fmt::format("{:.2f} {}\n", (double)i * US_PER_TICK, buckets[i]);
        }
    }

    /* 
     * Okay, peakmaxindex should now be a good candidate for the (or a) clock.
     * How this maps onto the actual clock rate depends on the encoding.
     */

    return peakmaxindex * NS_PER_TICK;
}

/* Decodes a fluxmap into a nice aligned array of bits. */
std::vector<bool> Fluxmap::decodeToBits(nanoseconds_t clockPeriod) const
{
    int pulses = duration() / clockPeriod;
    nanoseconds_t lowerThreshold = clockPeriod * clockDecodeThreshold;

    std::vector<bool> bitmap(pulses);
    unsigned count = 0;
    int cursor = 0;
    nanoseconds_t timestamp = 0;
    for (;;)
    {
        while (timestamp < lowerThreshold)
        {
            if (cursor >= bytes())
                goto abort;
            uint8_t interval = (*this)[cursor++];
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

nanoseconds_t BitmapDecoder::guessClock(Fluxmap& fluxmap) const
{
    return fluxmap.guessClock();
}

