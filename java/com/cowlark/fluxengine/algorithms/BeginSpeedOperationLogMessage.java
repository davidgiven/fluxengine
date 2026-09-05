package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We're starting to measure the drive's rotational speed, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record BeginSpeedOperationLogMessage() implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
        r.newline().add("Measuring rotational speed...").newline();
    }
}
