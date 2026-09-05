package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We've finished a read operation on a track, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record EndReadOperationLogMessage() implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
