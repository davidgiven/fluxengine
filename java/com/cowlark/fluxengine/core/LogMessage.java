package com.cowlark.fluxengine.core;

/**
 * A log message, ported from lib/core/logger.h. Each message type renders
 * itself to a LogRenderer.
 */
public interface LogMessage
{
    /* Renders this message. */
    void render(LogRenderer r);

    record StringMessage(String message) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add(message).newline();
        }
    }

    record ErrorLogMessage(String message) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add("Error:").add(message).newline();
        }
    }

    record EmergencyStopMessage() implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add("Stop!").newline();
        }
    }
}
