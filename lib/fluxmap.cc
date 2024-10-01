#include "lib/core/globals.h"
#include "lib/fluxmap.h"
#include "lib/decoders/fluxmapreader.h"
#include "protocol.h"

Fluxmap& Fluxmap::appendBytes(const Bytes& bytes)
{
    if (bytes.size() == 0)
        return *this;

    return appendBytes(&bytes[0], bytes.size());
}

Fluxmap& Fluxmap::appendBytes(const uint8_t* ptr, size_t len)
{
    ByteWriter bw(_bytes);
    bw.seekToEnd();

    while (len--)
    {
        uint8_t byte = *ptr++;
        _ticks += byte & 0x3f;
        bw.write_8(byte);
    }

    _duration = _ticks * NS_PER_TICK;
    return *this;
}

uint8_t& Fluxmap::findLastByte()
{
    if (_bytes.empty())
        appendByte(0x00);
    return *(_bytes.end() - 1);
}

Fluxmap& Fluxmap::appendInterval(uint32_t ticks)
{
    while (ticks >= 0x3f)
    {
        appendByte(0x3f);
        ticks -= 0x3f;
    }
    appendByte((uint8_t)ticks);
    return *this;
}

Fluxmap& Fluxmap::appendPulse()
{
    findLastByte() |= 0x80;
    return *this;
}

Fluxmap& Fluxmap::appendIndex()
{
    findLastByte() |= 0x40;
    return *this;
}

Fluxmap& Fluxmap::appendDesync()
{
    appendByte(F_DESYNC);
    return *this;
}

std::vector<std::unique_ptr<const Fluxmap>> Fluxmap::split() const
{
    std::vector<std::unique_ptr<const Fluxmap>> maps;
    auto bytesVector = rawBytes().split(F_DESYNC);

    for (auto bytes : bytesVector)
    {
        if (bytes.size() != 0)
            maps.push_back(std::move(std::make_unique<Fluxmap>(bytes)));
    }

    return maps;
}

/*
 * Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
Fluxmap::ClockData Fluxmap::guessClock(
    double noiseFloorFactor, double signalLevelFactor) const
{
    ClockData data = {};

    FluxmapReader fr(*this);

    while (!fr.eof())
    {
        unsigned interval;
        fr.findEvent(F_BIT_PULSE, interval);
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
