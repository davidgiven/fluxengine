package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.external.FluxEngine;
import com.google.common.collect.ImmutableList;
import java.util.List;

/**
 * A stream of flux transitions, ported from lib/data/fluxmap.{h,cc}.
 */
public class Fluxmap
{
    public record Position(int bytes, int ticks, int zeroes)
    {
        public long ns()
        {
            return (long) (ticks * FluxEngine.NS_PER_TICK);
        }

        @Override
        public String toString()
        {
            return String.format("[b:%d, t:%d, z:%d]", bytes, ticks, zeroes);
        }
    }

    private long duration;
    private int ticks;
    private Bytes bytes;
    private ImmutableList<Long> indexMarks;

    public Fluxmap()
    {
        bytes = new Bytes();
    }

    public Fluxmap(String s)
    {
        this();
        appendBytes(new Bytes(s));
    }

    public Fluxmap(Bytes bytes)
    {
        this();
        appendBytes(bytes);
    }

    public long duration()
    {
        return duration;
    }

    public int ticks()
    {
        return ticks;
    }

    public int bytes()
    {
        return bytes.size();
    }

    public Bytes rawBytes()
    {
        return bytes;
    }

    public Fluxmap appendInterval(int ticks)
    {
        while (ticks >= 0x3f)
        {
            appendByte(0x3f);
            ticks -= 0x3f;
        }
        appendByte(ticks & 0xff);
        return this;
    }

    public Fluxmap appendPulse()
    {
        ensureLastByte();
        int index = bytes.size() - 1;
        bytes.setByte(index, (byte) (bytes.getByte(index) | FluxEngine.F_BIT_PULSE));
        return this;
    }

    public Fluxmap appendIndex()
    {
        flushIndexMarks();
        ensureLastByte();
        int index = bytes.size() - 1;
        bytes.setByte(index, (byte) (bytes.getByte(index) | FluxEngine.F_BIT_INDEX));
        return this;
    }

    public Fluxmap appendDesync()
    {
        appendByte(FluxEngine.F_DESYNC);
        return this;
    }

    public Fluxmap appendBytes(Bytes data)
    {
        if (data.isEmpty())
            return this;

        flushIndexMarks();

        ByteWriter bw = new ByteWriter(bytes);
        bw.seekToEnd();
        for (int i = 0; i < data.size(); i++)
        {
            int b = data.getByte(i) & 0xff;
            ticks += b & 0x3f;
            bw.write8(b);
        }

        duration = (long) (ticks * FluxEngine.NS_PER_TICK);
        return this;
    }

    public Fluxmap appendByte(int b)
    {
        return appendBytes(Bytes.of(b));
    }

    public Fluxmap appendBits(List<Boolean> bits, long clock)
    {
        long now = duration;
        for (boolean bit : bits)
        {
            now += clock;
            if (bit)
            {
                int delta = (int) ((now - duration) / FluxEngine.NS_PER_TICK);
                appendInterval(delta);
                appendPulse();
            }
        }
        int delta = (int) ((now - duration) / FluxEngine.NS_PER_TICK);
        if (delta != 0)
            appendInterval(delta);
        return this;
    }

    public ImmutableList<Fluxmap> split()
    {
        ImmutableList.Builder<Fluxmap> maps = ImmutableList.builder();
        for (Bytes piece : bytes.split(FluxEngine.F_DESYNC))
        {
            if (!piece.isEmpty())
                maps.add(new Fluxmap(piece));
        }
        return maps.build();
    }

    public ImmutableList<Long> getIndexMarks()
    {
        if (indexMarks == null)
        {
            ImmutableList.Builder<Long> marks = ImmutableList.builder();
            long totalTicks = 0;
            long oldt = -1;
            for (int i = 0; i < bytes.size(); i++)
            {
                int b = bytes.getByte(i) & 0xff;
                totalTicks += b & 0x3f;
                if ((b & FluxEngine.F_BIT_INDEX) != 0)
                {
                    long t = (long) (totalTicks * FluxEngine.NS_PER_TICK);
                    if (t != oldt)
                        marks.add(t);
                    oldt = t;
                }
            }
            indexMarks = marks.build();
        }
        return indexMarks;
    }

    private void ensureLastByte()
    {
        if (bytes.isEmpty())
            appendByte(0x00);
    }

    private void flushIndexMarks()
    {
        indexMarks = null;
    }
}
