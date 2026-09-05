package com.cowlark.fluxengine.core;

import com.cowlark.fluxengine.core.LogMessage.StringMessage;
import java.util.function.Consumer;

/**
 * The logger, ported from lib/core/logger.{h,cc}.
 */
public final class Logger
{
    private static final ThreadLocal<Consumer<? super LogMessage>> loggerImpl =
            ThreadLocal.withInitial(() -> message -> {
                throw new IllegalStateException("logging from a thread with no logger set");
            });

    private Logger()
    {
    }

    public static void logf(String message, Object... args)
    {
        log(new StringMessage(String.format(message, args)));
    }

    public static void log(LogMessage message)
    {
        loggerImpl.get().accept(message);
    }

    public static Consumer<? super LogMessage> getLogger()
    {
        return loggerImpl.get();
    }

    public static void setLogger(Consumer<? super LogMessage> callback)
    {
        loggerImpl.set(callback);
    }
}
