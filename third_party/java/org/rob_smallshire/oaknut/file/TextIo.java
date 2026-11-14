package org.rob_smallshire.oaknut.file;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Acorn text uses pound/broken-bar glyphs at the ASCII backtick/vertical-bar positions. */
public final class TextIo
{
    private TextIo()
    {
    }

    public static String decode(byte[] bytes, boolean universalNewlines)
    {
        StringBuilder text = new StringBuilder();
        for (byte b : bytes)
        {
            int c = b & 255;
            text.append(c == 0x60 ? '£' : c == 0x7c ? '¦' : (char) c);
        }
        String result = text.toString();
        return universalNewlines ? result.replace("\r\n", "\n").replace('\r', '\n') : result;
    }

    public static byte[] encode(String text, String newline)
    {
        if (newline != null && !newline.isEmpty())
            text = text.replace("\n", newline);
        byte[] bytes = new byte[text.length()];
        for (int i = 0; i < text.length(); i++)
        {
            char c = text.charAt(i);
            if (c == '£')
                bytes[i] = 0x60;
            else if (c == '¦')
                bytes[i] = 0x7c;
            else if (c <= 255)
                bytes[i] = (byte) c;
            else
                throw new IllegalArgumentException("Unencodable Acorn text");
        }
        return bytes;
    }
}
