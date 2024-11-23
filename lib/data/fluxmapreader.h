#ifndef FLUXMAPREADER_H
#define FLUXMAPREADER_H

#include "lib/data/fluxmap.h"
#include "lib/config/flags.h"
#include "protocol.h"

class DecoderProto;
class FluxMatcher;

class FluxmapReader
{
public:
    FluxmapReader(const Fluxmap& fluxmap);
    FluxmapReader(const Fluxmap&& fluxmap) = delete;

    void rewind()
    {
        _pos.bytes = 0;
        _pos.ticks = 0;
        _pos.zeroes = 0;
    }

    bool eof() const
    {
        return _pos.bytes == _size;
    }

    Fluxmap::Position tell() const
    {
        return _pos;
    }

    /* Important! You can only reliably seek to 1 bits. */
    void seek(const Fluxmap::Position& pos)
    {
        _pos = pos;
    }

    int getDuration(void)
    {
        return (_fluxmap.duration());
    }

    void getNextEvent(int& event, unsigned& ticks);
    void skipToEvent(int event);
    bool findEvent(int event, unsigned& ticks);
    unsigned readInterval(nanoseconds_t clock); /* with debounce support */

    /* Important! You can only reliably seek to 1 bits. */
    void seek(nanoseconds_t ns);
    void seekToByte(unsigned byte);

    void seekToIndexMark();
    nanoseconds_t seekToPattern(const FluxMatcher& pattern);
    nanoseconds_t seekToPattern(
        const FluxMatcher& pattern, const FluxMatcher*& matching);

    struct ClockData
    {
        nanoseconds_t median;
        uint32_t noiseFloor;
        uint32_t signalLevel;
        nanoseconds_t peakStart;
        nanoseconds_t peakEnd;
        uint32_t buckets[256];
    };

    ClockData guessClock(
        double noiseFloorFactor = 0.01, double signalLevelFactor = 0.05);

private:
    const Fluxmap& _fluxmap;
    const uint8_t* _bytes;
    const size_t _size;
    Fluxmap::Position _pos;
    const DecoderProto& _config;
};

#endif
