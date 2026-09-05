package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.data.Disk;

/**
 * We've just read a disk, ported from lib/algorithms/readerwriter.cc.
 */
public record DiskReadLogMessage(Disk disk) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
