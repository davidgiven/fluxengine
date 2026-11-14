package org.rob_smallshire.oaknut.file;

import java.nio.charset.StandardCharsets;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Strict field encodings prevent a failed serialization from replacing characters. */
public final class AcornEncoding
{
    private AcornEncoding()
    {
    }

    public static byte[] encode(String value, boolean ascii)
    {
        int limit = ascii ? 127 : 255;
        for (int i = 0; i < value.length(); i++)
            if (value.charAt(i) > limit)
                throw new IllegalArgumentException("Unencodable character in " + value);
        return value.getBytes(StandardCharsets.ISO_8859_1);
    }

    public static void checkTitle(String title, int max, boolean ascii)
    {
        encode(title, ascii);
        if (title.length() > max || title.chars().anyMatch(c -> c == 0 || c == 13))
            throw new IllegalArgumentException("Invalid title: " + title);
    }

    public static String decode(byte[] bytes, boolean old)
    {
        StringBuilder result = new StringBuilder();
        for (byte b : bytes)
        {
            int value = b & (old ? 127 : 255);
            if (value == 0 || value == 13)
                break;
            result.append((char) value);
        }
        return result.toString();
    }
}
