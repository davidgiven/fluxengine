package com.cowlark.fluxengine.core;

public class Utils
{
    public static void hexdump(java.io.PrintStream stream, Bytes buffer)
    {
        int pos = 0;

        while (pos < buffer.size())
        {
            stream.printf("%05x : ", pos);
            for (int i = 0; i < 16; i++)
            {
                if ((pos + i) < buffer.size())
                    stream.printf("%02x ", buffer.getByte(pos + i));
                else
                    stream.print("-- ");
            }
            stream.print(" : ");
            for (int i = 0; i < 16; i++)
            {
                if ((pos + i) >= buffer.size())
                    break;

                int c = buffer.getByte(pos + i) & 0xff;
                if ((c >= 32) && (c <= 126))
                    stream.print((char) c);
                else
                    stream.print('.');
            }
            stream.println();

            pos += 16;
        }
    }

    public static String quoteString(String s)
    {
        boolean spaces = s.contains(" ");
        if (!spaces && !s.contains("\\") && !s.contains("'") && !s.contains("\""))
            return s;

        StringBuilder ss = new StringBuilder();
        if (spaces)
            ss.append('"');

        for (int i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            if ((c == '\\') || (c == '"') || (c == '!'))
                ss.append('\\');
            ss.append(c);
        }

        if (spaces)
            ss.append('"');

        return ss.toString();
    }
}
