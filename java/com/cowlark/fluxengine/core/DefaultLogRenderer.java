package com.cowlark.fluxengine.core;

import java.io.PrintStream;

class DefaultLogRenderer extends LogRenderer
{
    private final PrintStream stream;
    private boolean header = false;
    private boolean newline = false;
    private boolean space = false;
    private int lineLen = 0;

    DefaultLogRenderer(PrintStream stream)
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
        space = !message.isEmpty() && Character.isWhitespace(message.charAt(message.length() - 1));
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
        space = !message.isEmpty() && Character.isWhitespace(message.charAt(message.length() - 1));
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
