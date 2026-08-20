package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We've just finished measuring the drive's rotational speed, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record EndSpeedOperationLogMessage(double rotationalPeriodNs) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
        r.newline().add(String.format(
                "Rotational period is %.1fms (%.1frpm)",
                rotationalPeriodNs / 1e6,
                60e9 / rotationalPeriodNs)).newline();
    }
}
