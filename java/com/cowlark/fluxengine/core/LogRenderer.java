package com.cowlark.fluxengine.core;

import java.io.PrintStream;

/**
 * Renders log messages to a stream, ported from lib/core/logrenderer.cc.
 */
public abstract class LogRenderer
{
    public static LogRenderer create(PrintStream stream)
    {
        return new PrintingLogRenderer(stream);
    }

    public LogRenderer add(LogMessage message)
    {
        message.render(this);
        return this;
    }

    public abstract LogRenderer add(String message);

    public abstract LogRenderer comma();

    public abstract LogRenderer header(String message);

    public abstract LogRenderer newline();

}
