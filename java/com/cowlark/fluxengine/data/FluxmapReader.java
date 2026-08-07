package com.cowlark.fluxengine.data;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.F_EOF;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.decoders.DecoderProto;
import java.time.Duration;

/**
 * A cursor over a Fluxmap's raw bytes.
 */
public class FluxmapReader
{
    private final Fluxmap fluxmap;
    private final Bytes bytes;
    private final int size;
    private final DecoderProto decoder;
    private int posBytes;
    private int posTicks;
    private int posZeroes;

    public FluxmapReader(Fluxmap fluxmap, DecoderProto decoder)
    {
        this.fluxmap = fluxmap;
        bytes = fluxmap.rawBytes();
        size = fluxmap.bytes();
        this.decoder = decoder;
        rewind();
    }

    public void rewind()
    {
        posBytes = 0;
        posTicks = 0;
        posZeroes = 0;
    }

    public boolean eof()
    {
        return posBytes == size;
    }

    public FluxPosition tell()
    {
        return new FluxPosition(posBytes, posTicks, posZeroes);
    }

    public void seek(FluxPosition pos)
    {
        posBytes = pos.bytes();
        posTicks = pos.ticks();
        posZeroes = pos.zeroes();
    }

    public Duration getDuration()
    {
        return Duration.ofNanos((long) (fluxmap.ticks() * NS_PER_TICK));
    }

    public int getCurrentEvent()
    {
        if (eof())
            return F_EOF;
        return bytes.getByte(posBytes) & 0xc0;
    }

    public Event getNextEvent()
    {
        long ticks = 0;
        while (!eof())
        {
            int b = bytes.getByte(posBytes++) & 0xff;
            ticks += b & 0x3f;
            if (b == 0 || (b & (F_BIT_PULSE | F_BIT_INDEX)) != 0)
            {
                posTicks += (int) ticks;
                return new Event(b & 0xc0, ticks);
            }
        }
        posTicks += (int) ticks;
        return new Event(F_EOF, ticks);
    }

    public void skipToEvent(int event)
    {
        findEvent(event);
    }

    public EventResult findEvent(int event)
    {
        long ticks = 0;
        while (!eof())
        {
            Event e = getNextEvent();
            ticks += e.ticks();
            if (e.event() == F_EOF)
                return new EventResult(false, ticks);
            if (event == e.event() || (event & e.event()) != 0)
                return new EventResult(true, ticks);
        }
        return new EventResult(false, ticks);
    }

    public long readInterval(long clockTicks)
    {
        long thresholdTicks = (long) (clockTicks * decoder.getPulseDebounceThreshold());
        long ticks = 0;
        while (ticks <= thresholdTicks)
        {
            EventResult r = findEvent(F_BIT_PULSE);
            if (!r.found())
                break;
            ticks += r.ticks();
        }
        return ticks;
    }

    public void seek(long ticks)
    {
        if (ticks < posTicks)
        {
            posTicks = 0;
            posBytes = 0;
        }
        while (!eof() && posTicks < ticks)
            getNextEvent();
        posZeroes = 0;
    }

    public void seekToByte(int b)
    {
        if (b < posBytes)
        {
            posTicks = 0;
            posBytes = 0;
        }
        while (!eof() && posBytes < b)
            getNextEvent();
        posZeroes = 0;
    }

    public void seekToIndexMark()
    {
        skipToEvent(F_BIT_INDEX);
        posZeroes = 0;
    }

    public ClockData guessClock()
    {
        return guessClock(0.01, 0.05);
    }

    public ClockData guessClock(double noiseFloorFactor, double signalLevelFactor)
    {
        ClockData data = new ClockData();
        while (!eof())
        {
            long intervalTicks = findEvent(F_BIT_PULSE).ticks();
            if (intervalTicks > 0xff)
                continue;
            data.buckets[(int) intervalTicks]++;
        }

        int max = Integer.MIN_VALUE;
        int min = Integer.MAX_VALUE;
        for (int b : data.buckets)
        {
            max = Math.max(max, b);
            min = Math.min(min, b);
        }
        data.noiseFloor = (int) (min + (max - min) * noiseFloorFactor);
        data.signalLevel = (int) (min + (max - min) * signalLevelFactor);

        int pulseindexTicks = 0;
        while (pulseindexTicks < 256)
        {
            if (data.buckets[pulseindexTicks] > data.signalLevel)
                break;
            pulseindexTicks++;
        }
        if (pulseindexTicks == 256)
            return data;

        int peakloTicks = pulseindexTicks;
        while (peakloTicks > 0)
        {
            if (data.buckets[peakloTicks] < data.noiseFloor)
                break;
            peakloTicks--;
        }

        int peakhiTicks = pulseindexTicks;
        while (peakhiTicks < 255)
        {
            if (data.buckets[peakhiTicks] < data.noiseFloor)
                break;
            peakhiTicks++;
        }

        int totalSize = 0;
        for (int i = peakloTicks; i < peakhiTicks; i++)
            totalSize += data.buckets[i];

        int count = 0;
        int medianTicks = peakloTicks;
        while (medianTicks < peakhiTicks)
        {
            count += data.buckets[medianTicks];
            if (count > totalSize / 2)
                break;
            medianTicks++;
        }

        data.peakStartTicks = peakloTicks;
        data.peakEndTicks = peakhiTicks;
        data.medianTicks = medianTicks;
        return data;
    }

    public record Event(int event, long ticks)
    {
    }

    public record EventResult(boolean found, long ticks)
    {
    }

    public static class ClockData
    {
        public long medianTicks;
        public int noiseFloor;
        public int signalLevel;
        public long peakStartTicks;
        public long peakEndTicks;
        public int[] buckets = new int[256];
    }
}
