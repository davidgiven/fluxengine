#ifndef FLUXMAPREADER_H
#define FLUXMAPREADER_H

#include "fluxmap.h"
#include "protocol.h"

class FluxPattern
{
public:
    FluxPattern(unsigned len, uint64_t pattern);

    bool matches(const unsigned* intervals, double& clock) const;

    unsigned bits;
    std::vector<unsigned> intervals;
    unsigned length;
};

class FluxmapReader
{
public:
    struct Position
    {
        unsigned bytes;
        unsigned ticks;
    };

public:
    FluxmapReader(const Fluxmap& fluxmap):
        _fluxmap(fluxmap),
        _bytes(fluxmap.ptr()),
        _size(fluxmap.bytes())
    {
        rewind();
    }

    FluxmapReader(const Fluxmap&& fluxmap) = delete;

    void rewind()
    {
        _pos.bytes = 0;
        _pos.ticks = 0;
        _pendingZeroBits = 0;
    }

    bool eof() const
    { return _pos.bytes == _size; }

    Position tell() const
    { return _pos; }

    void seek(const Position& pos)
    { _pos = pos; }

    nanoseconds_t tellNs() const
    { return _pos.ticks * NS_PER_TICK; }

    int readOpcode(unsigned& ticks);
    unsigned readNextMatchingOpcode(uint8_t opcode);

    void seek(nanoseconds_t ns);
    void seekToIndexMark();
    nanoseconds_t seekToPattern(const FluxPattern& pattern);

    bool readRawBit(nanoseconds_t clockPeriod);
    std::vector<bool> readRawBits(unsigned count, nanoseconds_t clockPeriod);

private:
    const Fluxmap& _fluxmap;
    const uint8_t* _bytes;
    const size_t _size;
    Position _pos;
    unsigned _pendingZeroBits;
};

#endif
