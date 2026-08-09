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

    record BeginSpeedOperationLogMessage() implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add("Measuring rotational speed...").newline();
        }
    }

    record EndSpeedOperationLogMessage(double rotationalPeriodNs) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add(String.format(
                    "Rotational period is %.1fms (%.1frpm)",
                    rotationalPeriodNs / 1e6,
                    60e9 / rotationalPeriodNs)).newline();
        }
    }

    record BeginReadOperationLogMessage(int track, int head) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.header(String.format("R%2d.%d: ", track, head));
        }
    }

    record EndReadOperationLogMessage() implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
        }
    }

    record BeginWriteOperationLogMessage(int track, int head) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.header(String.format("W%2d.%d: ", track, head));
        }
    }

    record EndWriteOperationLogMessage() implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
        }
    }

    record BeginOperationLogMessage(String message) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
        }
    }

    record EndOperationLogMessage(String message) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
        }
    }

    record OperationProgressLogMessage(int progress) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
        }
    }

    record OptionLogMessage(String message) implements LogMessage
    {
        @Override
        public void render(LogRenderer r)
        {
            r.newline().add("OPTION:").add(message).newline();
        }
    }
}
