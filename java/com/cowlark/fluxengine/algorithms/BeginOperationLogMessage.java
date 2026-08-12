package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;

/**
 * We're starting a large-scale operation, ported from
 * lib/algorithms/readerwriter.cc.
 */
public record BeginOperationLogMessage(String message) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
