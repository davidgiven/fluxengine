package com.cowlark.fluxengine.data;

/* A special-casing: the matcher walks a sliding window of the last
 * `intervals()` intervals and checks whether they match the pattern. */

/**
 * A matcher over a run of flux intervals, ported from lib/data/fluxpattern.h.
 */
public interface FluxMatcher
{
    /* Intervals is the window of candidate intervals, with `endIndex` one
     * past the newest (and most recently found) interval. The matcher
     * examines the last `intervals().size()` entries (i.e. from
     * `endIndex - intervals()` to `endIndex`); `match` receives the result.
     */

    boolean matches(long[] intervals, int endIndex, double clockDecodeThreshold, FluxMatch match);

    int intervals();
}