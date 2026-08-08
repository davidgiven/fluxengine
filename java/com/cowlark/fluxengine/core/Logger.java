package com.cowlark.fluxengine.core;

import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import java.util.function.Consumer;

/**
 * The logger, ported from lib/core/logger.{h,cc}.
 */
public final class Logger
{
    private static Consumer<? super LogMessage> loggerImpl =
            message -> LogRenderer.create(System.out).add(message);

    private Logger()
    {
    }

    public static void log(String message)
    {
        log(new StringMessage(message));
    }

    public static void log(LogMessage message)
    {
        loggerImpl.accept(message);
    }

    public static void setLogger(Consumer<? super LogMessage> callback)
    {
        loggerImpl = callback;
    }
}
