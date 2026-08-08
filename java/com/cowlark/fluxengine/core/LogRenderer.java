package com.cowlark.fluxengine.core;

import com.cowlark.fluxengine.core.LogMessage.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.BeginSpeedOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.BeginWriteOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.EmergencyStopMessage;
import com.cowlark.fluxengine.core.LogMessage.EndSpeedOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.ErrorLogMessage;
import com.cowlark.fluxengine.core.LogMessage.OptionLogMessage;
import java.io.PrintStream;

/**
 * Renders log messages to a stream, ported from lib/core/logrenderer.cc.
 */
public abstract class LogRenderer
{
    public static LogRenderer create(PrintStream stream)
    {
        return new LogRendererImpl(stream);
    }

    public LogRenderer add(LogMessage message)
    {
        return switch (message)
        {
            case ErrorLogMessage msg -> newline().add("Error:").add(msg.render()).newline();
            case EmergencyStopMessage msg -> newline().add("Stop!").newline();
            case BeginSpeedOperationLogMessage msg -> newline().add(msg.render()).newline();
            case EndSpeedOperationLogMessage msg -> newline().add(msg.render()).newline();
            case BeginReadOperationLogMessage msg ->
                    header(String.format("R%2d.%d: ", msg.track(), msg.head()));
            case BeginWriteOperationLogMessage msg ->
                    header(String.format("W%2d.%d: ", msg.track(), msg.head()));
            case OptionLogMessage msg -> newline().add("OPTION:").add(msg.render()).newline();
            default -> newline().add(message.render()).newline();
        };
    }

    public abstract LogRenderer add(String message);

    public abstract LogRenderer comma();

    public abstract LogRenderer header(String message);

    public abstract LogRenderer newline();

    private static class LogRendererImpl extends LogRenderer
    {
        private final PrintStream stream;
        private boolean header = false;
        private boolean newline = false;
        private boolean space = false;
        private int lineLen = 0;

        LogRendererImpl(PrintStream stream)
        {
            this.stream = stream;
        }

        private void indent()
        {
            stream.print("       ");
            lineLen = 7;
            space = true;
        }

        @Override
        public LogRenderer add(String message)
        {
            if (newline && !header)
                indent();

            if (!space)
            {
                stream.print(' ');
                lineLen++;
            }

            newline = false;
            header = false;

            lineLen += message.length();
            if (lineLen >= 80)
            {
                stream.println();
                indent();
            }
            stream.print(message);
            space = !message.isEmpty() &&
                    Character.isWhitespace(message.charAt(message.length() - 1));
            return this;
        }

        @Override
        public LogRenderer header(String message)
        {
            if (!newline)
                stream.println();
            stream.print(message);
            lineLen = message.length();
            header = true;
            newline = true;
            space = !message.isEmpty() &&
                    Character.isWhitespace(message.charAt(message.length() - 1));
            return this;
        }

        @Override
        public LogRenderer comma()
        {
            if (!newline || header)
            {
                stream.print(';');
                space = false;
            }
            return this;
        }

        @Override
        public LogRenderer newline()
        {
            if (!header)
            {
                if (!newline)
                    stream.println();

                lineLen = 0;
                header = false;
                newline = true;
                space = true;
            }
            return this;
        }
    }
}
