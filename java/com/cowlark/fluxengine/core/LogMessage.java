package com.cowlark.fluxengine.core;

/**
 * A log message, ported from lib/core/logger.h. This is a marker interface;
 * each message type renders itself to a string via {@link #render()}.
 */
public interface LogMessage
{
    /* Fallback rendering of this message to a string. */
    String render();

    record StringMessage(String message) implements LogMessage
    {
        @Override
        public String render()
        {
            return message;
        }
    }

    record ErrorLogMessage(String message) implements LogMessage
    {
        @Override
        public String render()
        {
            return message;
        }
    }

    record EmergencyStopMessage() implements LogMessage
    {
        @Override
        public String render()
        {
            return "";
        }
    }

    record BeginSpeedOperationLogMessage() implements LogMessage
    {
        @Override
        public String render()
        {
            return "Measuring rotational speed...";
        }
    }

    record EndSpeedOperationLogMessage(double rotationalPeriodNs) implements LogMessage
    {
        @Override
        public String render()
        {
            return String.format(
                    "Rotational period is %.1fms (%.1frpm)",
                    rotationalPeriodNs / 1e6,
                    60e9 / rotationalPeriodNs);
        }
    }

    record BeginReadOperationLogMessage(int track, int head) implements LogMessage
    {
        @Override
        public String render()
        {
            return String.format("R%2d.%d", track, head);
        }
    }

    record EndReadOperationLogMessage() implements LogMessage
    {
        @Override
        public String render()
        {
            return "";
        }
    }

    record BeginWriteOperationLogMessage(int track, int head) implements LogMessage
    {
        @Override
        public String render()
        {
            return String.format("W%2d.%d", track, head);
        }
    }

    record EndWriteOperationLogMessage() implements LogMessage
    {
        @Override
        public String render()
        {
            return "";
        }
    }

    record BeginOperationLogMessage(String message) implements LogMessage
    {
        @Override
        public String render()
        {
            return message;
        }
    }

    record EndOperationLogMessage(String message) implements LogMessage
    {
        @Override
        public String render()
        {
            return message;
        }
    }

    record OperationProgressLogMessage(int progress) implements LogMessage
    {
        @Override
        public String render()
        {
            return "";
        }
    }

    record OptionLogMessage(String message) implements LogMessage
    {
        @Override
        public String render()
        {
            return message;
        }
    }
}
