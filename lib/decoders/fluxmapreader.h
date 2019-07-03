#ifndef FLUXMAPREADER_H
#define FLUXMAPREADER_H

#include "fluxmap.h"
#include "protocol.h"
#include "flags.h"

extern FlagGroup fluxmapReaderFlags;

class FluxMatcher;

struct FluxMatch
{
    const FluxMatcher* matcher;
    unsigned intervals;
    double clock;
    unsigned zeroes;
};

class FluxMatcher
{
public:
    virtual ~FluxMatcher() {}

    /* Returns the number of intervals matched */
    virtual bool matches(const unsigned* intervals, FluxMatch& match) const = 0;
    virtual unsigned intervals() const = 0;
};

class FluxPattern : public FluxMatcher
{
public:
    FluxPattern(unsigned bits, uint64_t patterns);

    bool matches(const unsigned* intervals, FluxMatch& match) const override;

    unsigned intervals() const override
    { return _intervals.size(); }

private:
    std::vector<unsigned> _intervals;
    unsigned _length;
    unsigned _bits;
    unsigned _highzeroes;
    bool _lowzero = false;

public:
    friend void test_patternconstruction();
    friend void test_patternmatching();
};

class FluxMatchers : public FluxMatcher
{
public:
    FluxMatchers(const std::initializer_list<const FluxMatcher*> matchers);

    bool matches(const unsigned* intervals, FluxMatch& match) const override;

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
        _pos.zeroes = 0;
    }

    bool eof() const
    { return _pos.bytes == _size; }

    Fluxmap::Position tell() const
    { return _pos; }

    /* Important! You can only reliably seek to 1 bits. */
    void seek(const Fluxmap::Position& pos)
    {
        _pos = pos;
    }

    int readOpcode(unsigned& ticks);
    unsigned readNextMatchingOpcode(uint8_t opcode);
    unsigned readInterval(nanoseconds_t clock); /* with debounce support */

    /* Important! You can only reliably seek to 1 bits. */
    void seek(nanoseconds_t ns);

    void seekToIndexMark();
    nanoseconds_t seekToPattern(const FluxMatcher& pattern);
    nanoseconds_t seekToPattern(const FluxMatcher& pattern, const FluxMatcher*& matching);

    bool readRawBit(nanoseconds_t clockPeriod);
    std::vector<bool> readRawBits(unsigned count, nanoseconds_t clockPeriod);
    std::vector<bool> readRawBits(const Fluxmap::Position& until, nanoseconds_t clockPeriod);

private:
    const Fluxmap& _fluxmap;
    const uint8_t* _bytes;
    const size_t _size;
    Fluxmap::Position _pos;
};

#endif
