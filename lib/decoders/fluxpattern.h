#pragma once

#include "lib/core/utils.h"
#include "lib/data/fluxmap.h"
#include "lib/config/flags.h"
#include "protocol.h"

class FluxMatcher;
class DecoderProto;

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
    {
        return _intervals.size();
    }

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
    {
        return _intervals;
    }

private:
    unsigned _intervals;
    std::vector<const FluxMatcher*> _matchers;
};
