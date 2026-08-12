package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * A large-scale operation has made progress, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record OperationProgressLogMessage(int progress) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
