#include "globals.h"
#include "fluxmap.h"
#include "fluxmapreader.h"
#include "flags.h"
#include "protocol.h"
#include <numeric>
#include <math.h>

static DoubleFlag clockDecodeThreshold(
    { "--bit-error-threshold" },
    "Amount of error to tolerate in pulse timing.",
    0.20);

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

FluxPattern::FluxPattern(unsigned len, uint64_t pattern)
{
    const uint64_t TOPBIT = 1ULL << 63;

    assert(pattern != 0);
    bits = len;

    while (!(pattern & TOPBIT))
        pattern <<= 1;

    length = 0;
    while (pattern != TOPBIT)
    {
        unsigned interval = 0;
        do
        {
            pattern <<= 1;
            interval++;
        }
        while (!(pattern & TOPBIT));
        intervals.push_back(interval);
        length += interval;
    }
}

bool FluxPattern::matches(const unsigned* end, double& clock) const
{
    const unsigned* start = end - intervals.size();
    unsigned candidatelength = std::accumulate(start, end, 0);
    if (!candidatelength)
        return false;
    clock = (double)candidatelength / (double)length;

    for (unsigned i=0; i<intervals.size(); i++)
    {
        double ii = clock * (double)intervals[i];
        double ci = (double)start[i];
        double error = fabs(1.0 - ii/ci);
        if (error > clockDecodeThreshold)
            return false;
    }

    return true;
}

nanoseconds_t FluxmapReader::seekToPattern(const FluxPattern& pattern)
{
    unsigned length = pattern.intervals.size();
    unsigned candidates[length];
    Position positions[length];

    for (unsigned& i : candidates)
        i = 0;
    for (Position& p : positions)
        p = tell();

    while (!eof())
    {
        unsigned interval = readNextMatchingOpcode(F_OP_PULSE);

        Position current = positions[0];
        for (unsigned i=0; i<(length-1); i++)
        {
            positions[i] = positions[i+1];
            candidates[i] = candidates[i+1];
        }
        positions[length-1] = tell();
        candidates[length-1] = interval;

        double clock;
        if (pattern.matches(&candidates[length], clock))
        {
            seek(current);
            return clock * NS_PER_TICK;
        }
    }

    return 0;
}

bool FluxmapReader::readRawBit(nanoseconds_t clockPeriod)
{
    assert(clockPeriod != 0);

    if (_pendingZeroBits)
    {
        _pendingZeroBits--;
        return false;
    }

    unsigned interval = readNextMatchingOpcode(F_OP_PULSE);
    unsigned clockTicks = clockPeriod / NS_PER_TICK;
    double clocks = (double)interval / clockTicks;

    if (clocks < 1.0)
        clocks = 1.0;
    _pendingZeroBits = (int)round(clocks) - 1;
    return true;
}

std::vector<bool> FluxmapReader::readRawBits(unsigned count, nanoseconds_t clockPeriod)
{
    std::vector<bool> result;
    while (!eof() && count--)
        result.push_back(readRawBit(clockPeriod));
    return result;
}
