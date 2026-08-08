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
}
