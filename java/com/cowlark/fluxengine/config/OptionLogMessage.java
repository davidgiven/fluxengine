package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.google.common.base.Strings;

public record OptionLogMessage(String message, OptionProto option) implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
        r.newline().add("OPTION:");
        if (!Strings.isNullOrEmpty(message))
            r.add(message + ":");
        r.add(option.getComment()).newline();
    }
}
