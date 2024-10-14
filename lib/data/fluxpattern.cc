#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxpattern.h"
#include "lib/config/proto.h"
#include "protocol.h"
#include <numeric>
#include <math.h>
#include <strings.h>

FluxPattern::FluxPattern(unsigned bits, uint64_t pattern): _bits(bits)
{
    const uint64_t TOPBIT = 1ULL << 63;

    assert(pattern != 0);

    unsigned lowbit = findLowestSetBit(pattern) - 1;

    pattern <<= 64 - bits;
    _highzeroes = 0;
    while (!(pattern & TOPBIT))
    {
        pattern <<= 1;
        _highzeroes++;
    }

    _length = 0;
    while (pattern != TOPBIT)
    {
        unsigned interval = 0;
        do
        {
            pattern <<= 1;
            interval++;
        } while (!(pattern & TOPBIT));
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

bool FluxPattern::matches(const unsigned* end, FluxMatch& match) const
{
    const double clockDecodeThreshold =
        globalConfig()->decoder().bit_error_threshold();
    const unsigned* start = end - _intervals.size();
    unsigned candidatelength = std::accumulate(start, end - _lowzero, 0);
    if (!candidatelength)
        return false;
    match.clock = (double)candidatelength / (double)_length;

    unsigned exactIntervals = _intervals.size() - _lowzero;
    for (unsigned i = 0; i < exactIntervals; i++)
    {
        double ii = match.clock * (double)_intervals[i];
        double ci = (double)start[i];
        double error = fabs((ii - ci) / match.clock);
        if (error > clockDecodeThreshold)
            return false;
    }

    if (_lowzero)
    {
        double ii = match.clock * (double)_intervals[exactIntervals];
        double ci = (double)start[exactIntervals];
        double error = (ii - ci) / match.clock;
        if (error > clockDecodeThreshold)
            return false;
    }

    match.matcher = this;
    match.intervals = _intervals.size();
    match.zeroes = _highzeroes;
    return true;
}

FluxMatchers::FluxMatchers(
    const std::initializer_list<const FluxMatcher*> matchers):
    _matchers(matchers)
{
    _intervals = 0;
    for (const auto* matcher : matchers)
        _intervals = std::max(_intervals, matcher->intervals());
}

bool FluxMatchers::matches(const unsigned* intervals, FluxMatch& match) const
{
    for (const auto* matcher : _matchers)
    {
        if (matcher->matches(intervals, match))
            return true;
    }
    return false;
}
