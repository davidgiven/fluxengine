package com.cowlark.fluxengine.data;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.F_DESYNC;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableList;
import java.util.List;

/**
 * A stream of flux transitions, ported from lib/data/fluxmap.{h,cc}.
 */
public class Fluxmap
{

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

    public int ticks()
    {
        return ticks;
    }

    /* The duration of the fluxmap in nanoseconds, ported from
     * lib/data/fluxmap.h Fluxmap::duration(). */
    public double durationNs()
    {
        return ticks * NS_PER_TICK;
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
        bytes.setByte(index, (byte) (bytes.getByte(index) | F_BIT_PULSE));
        return this;
    }

    public Fluxmap appendIndex()
    {
        flushIndexMarks();
        ensureLastByte();
        int index = bytes.size() - 1;
        bytes.setByte(index, (byte) (bytes.getByte(index) | F_BIT_INDEX));
        return this;
    }

    public Fluxmap appendDesync()
    {
        appendByte(F_DESYNC);
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

        return this;
    }

    public Fluxmap appendByte(int b)
    {
        return appendBytes(Bytes.of(b));
    }

    public Fluxmap appendBits(List<Boolean> bits, double clockNs)
    {
        double nowTicks = durationNs() / NS_PER_TICK;
        double clockTicks = clockNs / NS_PER_TICK;
        for (boolean bit : bits)
        {
            nowTicks += clockTicks;
            if (bit)
            {
                int deltaTicks = (int) nowTicks - ticks;
                appendInterval(deltaTicks);
                appendPulse();
            }
        }
        int deltaTicks = (int) nowTicks - ticks;
        if (deltaTicks != 0)
            appendInterval(deltaTicks);
        return this;
    }

    public ImmutableList<Fluxmap> split()
    {
        ImmutableList.Builder<Fluxmap> maps = ImmutableList.builder();
        for (Bytes piece : bytes.split(F_DESYNC))
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
            long oldTicks = -1;
            for (int i = 0; i < bytes.size(); i++)
            {
                int b = bytes.getByte(i) & 0xff;
                totalTicks += b & 0x3f;
                if ((b & F_BIT_INDEX) != 0)
                {
                    if (totalTicks != oldTicks)
                        marks.add(totalTicks);
                    oldTicks = totalTicks;
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
