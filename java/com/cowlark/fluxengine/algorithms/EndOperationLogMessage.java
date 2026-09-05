package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We've finished a large-scale operation, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record EndOperationLogMessage(String message) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
