package com.cowlark.fluxengine.data;

import java.util.Arrays;
import java.util.List;

/**
 * A compound flux matcher that tries several matchers in turn, ported from
 * lib/data/fluxpattern.{h,cc}.
 */
public class FluxMatchers implements FluxMatcher
{
    private final List<FluxMatcher> matchers;
    private final int intervalCount;

    public FluxMatchers(List<FluxMatcher> matchers)
    {
        this.matchers = matchers;
        intervalCount = matchers.stream()
                .mapToInt(FluxMatcher::intervals)
                .max()
                .orElse(0);
    }

    public static FluxMatchers of(FluxMatcher... matchers)
    {
        return new FluxMatchers(Arrays.asList(matchers));
    }

    @Override
    public boolean matches(long[] candidates, int endIndex, double clockDecodeThreshold,
            FluxMatch match)
    {
        for (FluxMatcher matcher : matchers)
        {
            if (matcher.matches(candidates, endIndex, clockDecodeThreshold, match))
                return true;
        }
        return false;
    }

    @Override
    public int intervals()
    {
        return intervalCount;
    }
}