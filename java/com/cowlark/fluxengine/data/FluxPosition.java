package com.cowlark.fluxengine.data;

import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import java.time.Duration;

public record FluxPosition(int bytes, int ticks, int zeroes)
{
    public Duration getDuration()
    {
        return Duration.ofNanos((long) (ticks * NS_PER_TICK));
    }

    @Override
    public String toString()
    {
        return String.format("[b:%d, t:%d, z:%d]", bytes, ticks, zeroes);
    }
}
