#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "flags.h"
#include "protocol.h"
#include "fmt/format.h"
#include <numeric>
#include <math.h>

static DoubleFlag clockDecodeThreshold(
    { "--bit-error-threshold" },
    "Amount of error to tolerate in pulse timing.",
    0.30);

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
    FluxMatcher(bits)
{
    const uint64_t TOPBIT = 1ULL << 63;

    assert(pattern != 0);

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
}

unsigned FluxPattern::matches(const unsigned* end, double& clock) const
{
    const unsigned* start = end - _intervals.size();
    unsigned candidatelength = std::accumulate(start, end, 0);
    if (!candidatelength)
        return 0;
    clock = (double)candidatelength / (double)_length;

    for (unsigned i=0; i<_intervals.size(); i++)
    {
        double ii = clock * (double)_intervals[i];
        double ci = (double)start[i];
        double error = fabs(1.0 - ii/ci);
        if (error > clockDecodeThreshold)
            return 0;
    }

    return _intervals.size();
}

FluxPatterns::FluxPatterns(unsigned bits, std::initializer_list<uint64_t> patterns):
    FluxMatcher(bits)
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

    for (unsigned& i : candidates)
        i = 0;
    for (Fluxmap::Position& p : positions)
        p = tell();

    while (!eof())
    {
        unsigned interval = readNextMatchingOpcode(F_OP_PULSE);

        for (unsigned i=0; i<intervalCount; i++)
        {
            positions[i] = positions[i+1];
            candidates[i] = candidates[i+1];
        }
        positions[intervalCount] = tell();
        candidates[intervalCount] = interval;

        double clock;
        unsigned m = pattern.matches(&candidates[intervalCount+1], clock);
        if (m)
        {
            seek(positions[intervalCount-m]);
            return clock * NS_PER_TICK;
        }
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
