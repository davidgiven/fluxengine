#ifndef FLUXMAPREADER_H
#define FLUXMAPREADER_H

#include "fluxmap.h"
#include "protocol.h"

class FluxMatcher
{
public:
    FluxMatcher(unsigned bits):
        _bits(bits)
    {}

    virtual ~FluxMatcher() {}

    /* Returns the number of intervals matched */
    virtual unsigned matches(const unsigned* intervals, double& clock) const = 0;
    virtual unsigned intervals() const = 0;

    unsigned bits() const
    { return _bits; }

protected:
    unsigned _bits;
};

class FluxPattern : public FluxMatcher
{
public:
    FluxPattern(unsigned bits, uint64_t patterns);

    unsigned matches(const unsigned* intervals, double& clock) const override;

    unsigned intervals() const override
    { return _intervals.size(); }

private:
    std::vector<unsigned> _intervals;
    unsigned _length;

public:
    friend void test_patternconstruction();
    friend void test_patternmatching();
};

class FluxPatterns : public FluxMatcher
{
public:
    FluxPatterns(unsigned bits, std::initializer_list<uint64_t> patterns);

    unsigned matches(const unsigned* intervals, double& clock) const override;

    unsigned intervals() const override
    { return _intervals; }

private:
    unsigned _intervals;
    std::vector<std::unique_ptr<FluxPattern>> _patterns;
};

class FluxmapReader
{
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

    Fluxmap::Position tell() const
    { return _pos; }

    void seek(const Fluxmap::Position& pos)
    { _pos = pos; _pendingZeroBits = 0; }

    int readOpcode(unsigned& ticks);
    unsigned readNextMatchingOpcode(uint8_t opcode);

    void seek(nanoseconds_t ns);
    void seekToIndexMark();
    nanoseconds_t seekToPattern(const FluxMatcher& pattern);

    bool readRawBit(nanoseconds_t clockPeriod);
    std::vector<bool> readRawBits(unsigned count, nanoseconds_t clockPeriod);
    std::vector<bool> readRawBits(const Fluxmap::Position& until, nanoseconds_t clockPeriod);

private:
    const Fluxmap& _fluxmap;
    const uint8_t* _bytes;
    const size_t _size;
    Fluxmap::Position _pos;
    unsigned _pendingZeroBits;
};

#endif
