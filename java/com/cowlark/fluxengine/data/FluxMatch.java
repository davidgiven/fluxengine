package com.cowlark.fluxengine.data;

/**
 * The result of matching a pattern against a run of flux intervals, ported
 * from lib/data/fluxpattern.h.
 */
public class FluxMatch
{
    public FluxMatcher matcher = null;
    public int intervals = 0;
    public double clock = 0.0;
    public int zeroes = 0;
}