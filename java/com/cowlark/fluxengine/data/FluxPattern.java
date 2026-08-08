package com.cowlark.fluxengine.data;

import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * A single flux pattern, ported from lib/data/fluxpattern.{h,cc}.
 */
public class FluxPattern implements FluxMatcher
{
    private static final long TOPBIT = 1L << 63;

    private final int bitCount;
    private final int highZeroes;
    private final List<Integer> intervals = new ArrayList<>();
    private final int length;
    private boolean lowZero = false;

    public FluxPattern(int bits, long pattern)
    {
        bitCount = bits;
        if (pattern == 0)
            throw new IllegalArgumentException("flux pattern may not be zero");
        if (bits < 1 || bits > 64)
            throw new IllegalArgumentException("flux pattern bit count must be 1..64");

        int lowBit = findLowestSetBit(pattern) - 1;

        pattern <<= (64 - bits);
        int highZeroesLocal = 0;
        while ((pattern & TOPBIT) == 0)
        {
            pattern <<= 1;
            highZeroesLocal++;
        }
        highZeroes = highZeroesLocal;

        int lengthLocal = 0;
        while (pattern != TOPBIT)
        {
            int interval = 0;
            do
            {
                pattern <<= 1;
                interval++;
            } while ((pattern & TOPBIT) == 0);
            intervals.add(interval);
            lengthLocal += interval;
        }
        length = lengthLocal;

        if (lowBit != 0)
        {
            lowZero = true;
            intervals.add(lowBit + 1);
        }
    }

    /* Returns the index (1-based) of the lowest set bit, or 0 if none. */
    private static int findLowestSetBit(long value)
    {
        if (value == 0)
            return 0;
        int bit = 1;
        while ((value & 1) == 0)
        {
            value >>= 1;
            bit++;
        }
        return bit;
    }

    @Override
    /* The `endIndex` is one past the newest candidate interval, mirroring the
     * C++ pointer passed as `&*candidates.end()`. */
    public boolean matches(long[] candidates, int endIndex, double clockDecodeThreshold,
            FluxMatch match)
    {
        int start = endIndex - intervals.size();

        int candidateLength = 0;
        for (int i = start; i < endIndex - (lowZero ? 1 : 0); i++)
            candidateLength += candidates[i];

        if (candidateLength == 0)
            return false;
        match.clock = (double) candidateLength / (double) length;

        int exactIntervals = intervals.size() - (lowZero ? 1 : 0);
        for (int i = 0; i < exactIntervals; i++)
        {
            double ii = match.clock * intervals.get(i);
            double ci = candidates[start + i];

            double error = Math.abs((ii - ci) / match.clock);
            if (error > clockDecodeThreshold)
                return false;
        }

        if (lowZero)
        {
            double ii = match.clock * intervals.get(exactIntervals);
            double ci = candidates[start + exactIntervals];

            double error = (ii - ci) / match.clock;
            if (error > clockDecodeThreshold)
                return false;
        }

        match.matcher = this;
        match.intervals = intervals.size();
        match.zeroes = highZeroes;
        return true;
    }

    @Override
    public int intervals()
    {
        return intervals.size();
    }

    /* Package-private accessors for the tests (mirrors the C++ `friend`
     * test_patternconstruction/test_patternmatching). */

    int getBitCount()
    {
        return bitCount;
    }

    List<Integer> getIntervals()
    {
        return Collections.unmodifiableList(intervals);
    }

    int getHighZeroes()
    {
        return highZeroes;
    }
}