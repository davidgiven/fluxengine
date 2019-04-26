#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "flags.h"
#include "protocol.h"
#include "fmt/format.h"
#include <numeric>
#include <math.h>
#include <strings.h>

static DoubleFlag clockDecodeThreshold(
    { "--bit-error-threshold" },
    "Amount of error to tolerate in pulse timing, in fractions of a clock.",
    0.20);

static DoubleFlag clockIntervalBias(
    { "--clock-interval-bias" },
    "Adjust intervals between pulses by this many clocks before decoding.",
    -0.02);

int FluxmapReader::readOpcode(unsigned& ticks)
{
    ticks = 0;

    while (!eof())
    {
        uint8_t b = _bytes[_pos.bytes++];
        if (b < 0x80)
            ticks += b;
        else
        {
            _pos.ticks += ticks;
            return b;
        }
    }

    _pos.ticks += ticks;
    return -1;
}

unsigned FluxmapReader::readNextMatchingOpcode(uint8_t opcode)
{
    unsigned ticks = 0;

    for (;;)
    {
        unsigned thisTicks;
        int op = readOpcode(thisTicks);
        ticks += thisTicks;
        if (op == -1)
            return 0;
        if (op == opcode)
            return ticks;
    }
}

FluxPattern::FluxPattern(unsigned bits, uint64_t pattern):
    _bits(bits)
{
    const uint64_t TOPBIT = 1ULL << 63;

    assert(pattern != 0);

    unsigned lowbit = ffsll(pattern)-1;
    while (!(pattern & TOPBIT))
        pattern <<= 1;

    _length = 0;
    while (pattern != TOPBIT)
    {
        unsigned interval = 0;
        do
        {
            pattern <<= 1;
            interval++;
        }
        while (!(pattern & TOPBIT));
        _intervals.push_back(interval);
        _length += interval;
    }

    if (lowbit)
    {
        _lowzero = true;
        /* Note that length does *not* include this interval. */
        _intervals.push_back(lowbit + 1);
    }
}

unsigned FluxPattern::matches(const unsigned* end, double& clock) const
{
    const unsigned* start = end - _intervals.size();
    unsigned candidatelength = std::accumulate(start, end - _lowzero, 0);
    if (!candidatelength)
        return 0;
    clock = (double)candidatelength / (double)_length;

    unsigned exactIntervals = _intervals.size() - _lowzero;
    for (unsigned i=0; i<exactIntervals; i++)
    {
        double ii = clock * (double)_intervals[i];
        double ci = (double)start[i];
        double error = fabs((ii - ci) / clock);
        if (error > clockDecodeThreshold)
            return 0;
    }

    if (_lowzero)
    {
        double ii = clock * (double)_intervals[exactIntervals];
        double ci = (double)start[exactIntervals];
        double error = (ii - ci) / clock;
        if (error > clockDecodeThreshold)
            return 0;
    }

    return _intervals.size();
}

FluxPatterns::FluxPatterns(unsigned bits, std::initializer_list<uint64_t> patterns)
{
    _intervals = 0;
    for (uint64_t p : patterns)
    {
        auto pattern = std::make_unique<FluxPattern>(bits, p);
        _intervals = std::max(_intervals, pattern->intervals());
        _patterns.push_back(std::move(pattern));
    }
}

unsigned FluxPatterns::matches(const unsigned* intervals, double& clock) const
{
    for (const auto& pattern : _patterns)
    {
        unsigned m = pattern->matches(intervals, clock);
        if (m)
            return m;
    }
    return 0;
}

FluxMatchers::FluxMatchers(const std::initializer_list<const FluxMatcher*> matchers):
    _matchers(matchers)
{
    _intervals = 0;
    for (const auto* matcher : matchers)
        _intervals = std::max(_intervals, matcher->intervals());
}

unsigned FluxMatchers::matches(const unsigned* intervals, double& clock) const
{
    for (const auto* matcher : _matchers)
    {
        unsigned m = matcher->matches(intervals, clock);
        if (m)
            return m;
    }
    return 0;
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
        unsigned t;
        readOpcode(t);
    }
}

nanoseconds_t FluxmapReader::seekToPattern(const FluxMatcher& pattern)
{
    unsigned intervalCount = pattern.intervals();
    unsigned candidates[intervalCount+1];
    Fluxmap::Position positions[intervalCount+1];

    for (unsigned i=0; i<=intervalCount; i++)
    {
        positions[i] = tell();
        candidates[i] = 0;
    }

    while (!eof())
    {
        double clock;
        unsigned m = pattern.matches(&candidates[intervalCount+1], clock);
        if (m)
        {
            seek(positions[intervalCount-m]);
            return clock * NS_PER_TICK;
        }

        for (unsigned i=0; i<intervalCount; i++)
        {
            positions[i] = positions[i+1];
            candidates[i] = candidates[i+1];
        }
        candidates[intervalCount] = readNextMatchingOpcode(F_OP_PULSE);
        positions[intervalCount] = tell();

    }

    return 0;
}

void FluxmapReader::seekToIndexMark()
{
    readNextMatchingOpcode(F_OP_INDEX);
    _pendingZeroBits = 0;
}

bool FluxmapReader::readRawBit(nanoseconds_t clockPeriod)
{
    assert(clockPeriod != 0);

    if (_pendingZeroBits)
    {
        _pendingZeroBits--;
        return false;
    }

    nanoseconds_t interval = readNextMatchingOpcode(F_OP_PULSE)*NS_PER_TICK;
    double clocks = (double)interval / clockPeriod + clockIntervalBias;

    if (clocks < 1.0)
        clocks = 1.0;
    _pendingZeroBits = (int)round(clocks) - 1;
    return true;
}

std::vector<bool> FluxmapReader::readRawBits(unsigned count, nanoseconds_t clockPeriod)
{
    std::vector<bool> result;
    while (!eof() && count--)
    {
        bool b = readRawBit(clockPeriod);
        result.push_back(b);
    }
    return result;
}

std::vector<bool> FluxmapReader::readRawBits(const Fluxmap::Position& until, nanoseconds_t clockPeriod)
{
    std::vector<bool> result;
    while (!eof() && (_pos.bytes < until.bytes))
        result.push_back(readRawBit(clockPeriod));
    return result;
}
