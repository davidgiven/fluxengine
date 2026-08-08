package com.cowlark.fluxengine.core;

import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import java.util.function.Consumer;

/**
 * The logger, ported from lib/core/logger.{h,cc}.
 */
public final class Logger
{
    private static Consumer<? super LogMessage> loggerImpl =
            new DefaultLogRenderer(System.out)::add;

    private Logger()
    {
    }

    public static void logf(String message, Object... args)
    {
        log(new StringMessage(String.format(message, args)));
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
