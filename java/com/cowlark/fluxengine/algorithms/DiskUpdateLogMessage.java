package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.data.Disk;

public record DiskUpdateLogMessage(Disk disk) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
    }
}
