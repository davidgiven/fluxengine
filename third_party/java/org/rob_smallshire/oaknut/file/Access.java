package org.rob_smallshire.oaknut.file;

import java.util.Locale;

/* Copyright (c) 2026 Robert Smallshire. MIT license; see ../LICENSE.
 * Canonical Acorn OSFILE attributes, distinct from the packed directory bits. */
public final class Access
{
    public static final int R = 1, W = 2, E = 4, L = 8, PR = 16, PW = 32, PE = 64, PL = 128;
    public static final int SYSTEM = 256, HIDDEN = 512, ARCHIVE = 1024;

    private Access()
    {
    }

    public static int parse(String text)
    {
        String s = text.strip().toUpperCase(Locale.ROOT);
        if (s.startsWith("0X"))
            return Integer.parseInt(s.substring(2), 16);
        if (s.matches("[0-9A-F]{2}") && !s.equals("EE"))
            return Integer.parseInt(s, 16);
        String[] halves = s.split("/", -1);
        if (halves.length > 2)
            throw new IllegalArgumentException("Invalid access: " + text);
        int bits = 0;
        for (int part = 0; part < halves.length; part++)
            for (char c : halves[part].toCharArray())
                bits |= switch (c)
                {
                    case 'R' -> part == 0 ? R : PR;
                    case 'W' -> part == 0 ? W : PW;
                    case 'E' -> part == 0 ? E : PE;
                    case 'L' -> part == 0 ? L : PL;
                    case 'P' ->
                    {
                        if (part != 0)
                            throw new IllegalArgumentException(text);
                        yield PL;
                    }
                    default -> throw new IllegalArgumentException("Invalid access: " + text);
                };
        return bits;
    }

    public static String format(int bits)
    {
        StringBuilder result = new StringBuilder();
        int[] flags = {PL, L, W, R, E, PW, PR, PE};
        char[] names = {'P', 'L', 'W', 'R', 'E', 'W', 'R', 'E'};
        for (int i = 0; i < flags.length; i++)
        {
            if (i == 5)
                result.append('/');
            if ((bits & flags[i]) != 0)
                result.append(names[i]);
        }
        return result.toString();
    }

    public static boolean isRunOnly(int bits)
    {
        return (bits & (R | E)) == E;
    }    public static final int WR = R | W, LWR = L | WR;




}
