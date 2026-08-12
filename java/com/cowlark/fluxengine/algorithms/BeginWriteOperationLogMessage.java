package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We're starting a write operation on a track, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record BeginWriteOperationLogMessage(int track, int head) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
        r.header(String.format("W%2d.%d: ", track, head));
    }
}
