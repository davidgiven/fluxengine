#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/data/fluxpattern.h"
#include "lib/config/proto.h"
#include "protocol.h"
#include <numeric>
#include <math.h>
#include <strings.h>

FluxmapReader::FluxmapReader(const Fluxmap& fluxmap):
    _fluxmap(fluxmap),
    _bytes(fluxmap.ptr()),
    _size(fluxmap.bytes()),
    _config(globalConfig()->decoder())
{
    rewind();
}

void FluxmapReader::getNextEvent(int& event, unsigned& ticks)
{
    ticks = 0;

    while (!eof())
    {
        uint8_t b = _bytes[_pos.bytes++];
        ticks += b & 0x3f;
        if (!b || (b & (F_BIT_PULSE | F_BIT_INDEX)))
        {
            _pos.ticks += ticks;
            event = b & 0xc0;
            return;
        }
    }

    _pos.ticks += ticks;
    event = F_EOF;
}

void FluxmapReader::skipToEvent(int event)
{
    unsigned ticks;
    findEvent(event, ticks);
}

bool FluxmapReader::findEvent(int event, unsigned& ticks)
{
    ticks = 0;

    while (!eof())
    {
        unsigned thisTicks;
        int thisEvent;
        getNextEvent(thisEvent, thisTicks);
        ticks += thisTicks;

        if (thisEvent == F_EOF)
            return false;

        if ((event == thisEvent) || (event & thisEvent))
            return true;
    }

    return false;
}

unsigned FluxmapReader::readInterval(nanoseconds_t clock)
{
    unsigned thresholdTicks =
        (clock * _config.pulse_debounce_threshold()) / NS_PER_TICK;
    unsigned ticks = 0;

    while (ticks <= thresholdTicks)
    {
        unsigned thisTicks;
        if (!findEvent(F_BIT_PULSE, thisTicks))
            break;
        ticks += thisTicks;
    }
    return ticks;
}

void FluxmapReader::seek(nanoseconds_t ns)
{
    unsigned ticks = ns / NS_PER_TICK;
    if (ticks < _pos.ticks)
    {
        _pos.ticks = 0;
        _pos.bytes = 0;
    }

    while (!eof() && (_pos.ticks < ticks))
    {
        int e;
        unsigned t;
        getNextEvent(e, t);
    }
    _pos.zeroes = 0;
}

void FluxmapReader::seekToByte(unsigned b)
{
    if (b < _pos.bytes)
    {
        _pos.ticks = 0;
        _pos.bytes = 0;
    }

    while (!eof() && (_pos.bytes < b))
    {
        int e;
        unsigned t;
        getNextEvent(e, t);
    }
    _pos.zeroes = 0;
}

nanoseconds_t FluxmapReader::seekToPattern(const FluxMatcher& pattern)
{
    const FluxMatcher* unused;
    return seekToPattern(pattern, unused);
}

nanoseconds_t FluxmapReader::seekToPattern(
    const FluxMatcher& pattern, const FluxMatcher*& matching)
{
    unsigned intervalCount = pattern.intervals();
    std::vector<unsigned> candidates(intervalCount + 1);
    std::vector<Fluxmap::Position> positions(intervalCount + 1);

    for (unsigned i = 0; i <= intervalCount; i++)
    {
        positions[i] = tell();
        candidates[i] = 0;
    }

    while (!eof())
    {
        FluxMatch match;
        if (pattern.matches(&*candidates.end(), match))
        {
            seek(positions[intervalCount - match.intervals]);
            _pos.zeroes = match.zeroes;
            matching = match.matcher;
            nanoseconds_t detectedClock = match.clock * NS_PER_TICK;
            if (detectedClock > (_config.minimum_clock_us() * 1000))
                return match.clock * NS_PER_TICK;
        }

        for (unsigned i = 0; i < intervalCount; i++)
        {
            positions[i] = positions[i + 1];
            candidates[i] = candidates[i + 1];
        }
        findEvent(F_BIT_PULSE, candidates[intervalCount]);
        positions[intervalCount] = tell();
    }

    matching = NULL;
    return 0;
}

void FluxmapReader::seekToIndexMark()
{
    skipToEvent(F_BIT_INDEX);
    _pos.zeroes = 0;
}

/*
 * Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
FluxmapReader::ClockData FluxmapReader::guessClock(
    double noiseFloorFactor, double signalLevelFactor)
{
    ClockData data = {};

    while (!eof())
    {
        unsigned interval;
        findEvent(F_BIT_PULSE, interval);
        if (interval > 0xff)
            continue;
        data.buckets[interval]++;
    }

    uint32_t max =
        *std::max_element(std::begin(data.buckets), std::end(data.buckets));
    uint32_t min =
        *std::min_element(std::begin(data.buckets), std::end(data.buckets));
    data.noiseFloor = min + (max - min) * noiseFloorFactor;
    data.signalLevel = min + (max - min) * signalLevelFactor;

    /* Find a point solidly within the first pulse. */

    int pulseindex = 0;
    while (pulseindex < 256)
    {
        if (data.buckets[pulseindex] > data.signalLevel)
            break;
        pulseindex++;
    }
    if (pulseindex == -1)
        return data;

    /* Find the upper and lower bounds of the pulse. */

    int peaklo = pulseindex;
    while (peaklo > 0)
    {
        if (data.buckets[peaklo] < data.noiseFloor)
            break;
        peaklo--;
    }

    int peakhi = pulseindex;
    while (peakhi < 255)
    {
        if (data.buckets[peakhi] < data.noiseFloor)
            break;
        peakhi++;
    }

    /* Find the total accumulated size of the pulse. */

    uint32_t total_size = 0;
    for (int i = peaklo; i < peakhi; i++)
        total_size += data.buckets[i];

    /* Now find the median. */

    uint32_t count = 0;
    int median = peaklo;
    while (median < peakhi)
    {
        count += data.buckets[median];
        if (count > (total_size / 2))
            break;
        median++;
    }

    /*
     * Okay, the median should now be a good candidate for the (or a) clock.
     * How this maps onto the actual clock rate depends on the encoding.
     */

    data.peakStart = peaklo * NS_PER_TICK;
    data.peakEnd = peakhi * NS_PER_TICK;
    data.median = median * NS_PER_TICK;
    return data;
}
