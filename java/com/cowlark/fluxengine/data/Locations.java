package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;

/**
 * Parsing of cylinder/head location descriptor strings, ported from
 * lib/data/locations.cc.
 */
public class Locations
{
    private Locations()
    {
    }

    public static ImmutableList<CylinderHead> parseCylinderHeadsString(String s)
    {
        List<CylinderHead> result = new ArrayList<>();
        Parser parser = new Parser(s);
        parser.skipSpaces();
        while (!parser.eof())
        {
            result.addAll(parser.parseCh());
            parser.skipSpaces();
        }

        if (result.isEmpty())
            throw new FluxEngineException("track descriptor parse error: no locations specified");

        Collections.sort(result);
        return ImmutableList.copyOf(result);
    }

    public static String convertCylinderHeadsToString(List<CylinderHead> chs)
    {
        StringBuilder sb = new StringBuilder();
        boolean first = true;
        for (CylinderHead ch : chs)
        {
            if (!first)
                sb.append(' ');
            sb.append(String.format("c%dh%d", ch.cylinder(), ch.head()));
            first = false;
        }
        return sb.toString();
    }

    private static final class Parser
    {
        private final String s;
        private int pos;

        Parser(String s)
        {
            this.s = s;
        }

        boolean eof()
        {
            return pos >= s.length();
        }

        void skipSpaces()
        {
            while (pos < s.length() && s.charAt(pos) == ' ')
                pos++;
        }

        List<CylinderHead> parseCh()
        {
            expect('c');
            List<Integer> cylinders = parseMembers();
            expect('h');
            List<Integer> heads = parseMembers();

            List<CylinderHead> result = new ArrayList<>();
            for (int c : cylinders)
            {
                for (int h : heads)
                    result.add(new CylinderHead(c, h));
            }
            return result;
        }

        List<Integer> parseMembers()
        {
            List<Integer> result = new ArrayList<>();
            result.addAll(parseMember());
            while (peek() == ',')
            {
                pos++;
                result.addAll(parseMember());
            }
            return result;
        }

        List<Integer> parseMember()
        {
            int start = parseUnsigned();
            int end = start;
            int step = 1;
            if (peek() == '-')
            {
                pos++;
                end = parseUnsigned();
            }
            if (peek() == 'x')
            {
                pos++;
                step = parseUnsigned();
            }

            if (start < 0)
                throw error("range start " + start + " must be at least 0");
            if (end < start)
                throw error("range end " + end + " must be at least the start");
            if (step < 1)
                throw error("range step " + step + " must be at least one");

            List<Integer> result = new ArrayList<>();
            for (int i = start; i <= end; i += step)
                result.add(i);
            return result;
        }

        int parseUnsigned()
        {
            int start = pos;
            while (pos < s.length() && Character.isDigit(s.charAt(pos)))
                pos++;
            if (pos == start)
                throw error("expected a number at '" + pos + "'");
            try
            {
                return Integer.parseInt(s.substring(start, pos));
            } catch (NumberFormatException e)
            {
                throw error("number out of range at '" + start + "'");
            }
        }

        char peek()
        {
            return pos < s.length() ? s.charAt(pos) : '\0';
        }

        void expect(char c)
        {
            if (eof() || s.charAt(pos) != c)
                throw error("expected '" + c + "' at '" + pos + "'");
            pos++;
        }

        FluxEngineException error(String message)
        {
            return new FluxEngineException("track descriptor parse error: " + message);
        }
    }
}
