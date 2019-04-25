#ifndef FLUXMAPREADER_H
#define FLUXMAPREADER_H

#include "fluxmap.h"
#include "protocol.h"

class FluxMatcher
{
public:
    virtual ~FluxMatcher() {}

    /* Returns the number of intervals matched */
    virtual unsigned matches(const unsigned* intervals, double& clock) const = 0;
    virtual unsigned intervals() const = 0;
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
    unsigned _bits;
    bool _lowzero = false;

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

class FluxMatchers : public FluxMatcher
{
public:
    FluxMatchers(const std::initializer_list<const FluxMatcher*> matchers);

    unsigned matches(const unsigned* intervals, double& clock) const override;

    unsigned intervals() const override
    { return _intervals; }

private:
    unsigned _intervals;
    std::vector<const FluxMatcher*> _matchers;
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

    /* Important! You can only reliably seek to 1 bits. */
    void seek(const Fluxmap::Position& pos)
    {
        _pos = pos;
        _pendingZeroBits = 0;
    }

    int readOpcode(unsigned& ticks);
    unsigned readNextMatchingOpcode(uint8_t opcode);

    /* Important! You can only reliably seek to 1 bits. */
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
