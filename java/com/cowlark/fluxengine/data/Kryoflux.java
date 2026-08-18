package com.cowlark.fluxengine.data;

import static com.cowlark.fluxengine.external.FluxEngine.TICK_FREQUENCY;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.TreeSet;

/**
 * Reader for raw Kryoflux stream files, ported from lib/external/kryoflux.cc.
 * This file lives in the data package rather than the external one because it
 * constructs Fluxmap objects.
 */
public final class Kryoflux
{
    private static final double MCLK_HZ = ((18432000.0 * 73.0) / 14.0) / 2.0;
    private static final double SCLK_HZ = MCLK_HZ / 2;
    private static final double TICKS_PER_SCLK = TICK_FREQUENCY / SCLK_HZ;
    private static final double ICLK_HZ = MCLK_HZ / 16;

    private Kryoflux()
    {
    }

    public static Fluxmap readStream(String dir, int track, int side)
    {
        String suffix = String.format("%02d.%d.raw", track, side);

        File directory = new File(dir);
        if (!directory.isDirectory())
            error("cannot access path '%s'", dir);

        String filename = null;
        File[] files = directory.listFiles();
        if (files != null)
        {
            for (File file : files)
            {
                if (hasSuffix(file.getName(), suffix))
                {
                    if (filename != null)
                        error("data is ambiguous --- multiple files end in %s", suffix);
                    filename = dir + File.separator + file.getName();
                }
            }
        }

        if (filename == null)
            error("failed to find track %d side %d in %s", track, side, dir);

        return readStream(filename);
    }

    public static Fluxmap readStream(String filename)
    {
        try
        {
            return readStream(new Bytes(Files.readAllBytes(Path.of(filename))));
        } catch (IOException e)
        {
            throw new FluxEngineException(String.format("cannot open input file '%s': %s",
                    filename,
                    e.getMessage()));
        }
    }

    public static Fluxmap readStream(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);

        /* Pass 1: scan the stream looking for index marks. */

        TreeSet<Integer> indexmarks = new TreeSet<>();
        br.seek(0);
        pass1:
        while (!br.eof())
        {
            int b = br.read8();
            int len = 0;
            switch (b)
            {
                case 0x0d: /* OOB block */
                {
                    int blocktype = br.read8();
                    len = br.readLe16();
                    if (br.eof())
                        break pass1;

                    if (blocktype == 0x02)
                    {
                        /* index data, sent asynchronously */
                        int streampos = br.readLe32();
                        indexmarks.add(streampos);
                        len -= 4;
                    }
                    break;
                }

                default:
                {
                    if ((b >= 0x00) && (b <= 0x07))
                        len = 1; /* Flux2: double byte value */
                    else if (b == 0x08)
                        len = 0; /* Nop1: do nothing */
                    else if (b == 0x09)
                        len = 1; /* Nop2: skip one byte */
                    else if (b == 0x0a)
                        len = 2; /* Nop3: skip two bytes */
                    else if (b == 0x0b)
                        len = 0; /* Ovl16: the next block is 0x10000 sclks
                         * longer than normal. */
                    else if (b == 0x0c)
                        len = 2; /* Flux3: triple byte value */
                    else if ((b >= 0x0e) && (b <= 0xff))
                        len = 0; /* Flux1: single byte value */
                    else
                        error("unknown stream block byte 0x%01x at 0x%08x", b, (long) br.pos() - 1);
                }
            }
            br.skip(len);
        }

        /* Pass 2: actually read the data. */

        Fluxmap fluxmap = new Fluxmap();
        long extrasclks = 0;
        int streamdelta = 0;
        br.seek(0);
        pass2:
        while (!br.eof())
        {
            int b = br.read8();
            switch (b)
            {
                case 0x0d: /* OOB block */
                {
                    int blocktype = br.read8();
                    int blocklen = br.readLe16();
                    if (br.eof())
                        break pass2;

                    switch (blocktype)
                    {
                        case 0x01: /* streaminfo */
                        {
                            int blockpos = br.pos() - 3;
                            streamdelta = blockpos - br.readLe32();
                            blocklen -= 4;
                            break;
                        }
                    }

                    br.skip(blocklen);
                    break;
                }

                default:
                {
                    if ((b >= 0x00) && (b <= 0x07))
                    {
                        /* Flux2: double byte value */
                        b = (b << 8) | br.read8();
                        writeFlux(fluxmap, indexmarks, br, streamdelta, extrasclks + b);
                        extrasclks = 0;
                    } else if (b == 0x08)
                    {
                        /* Nop1: do nothing */
                    } else if (b == 0x09)
                    {
                        /* Nop2: skip one byte */
                        br.skip(1);
                    } else if (b == 0x0a)
                    {
                        /* Nop3: skip two bytes */
                        br.skip(2);
                    } else if (b == 0x0b)
                    {
                        /* Ovl16: the next flux value is 0x10000 sclks longer
                         * than normal. */
                        extrasclks += 0x10000;
                    } else if (b == 0x0c)
                    {
                        /* Flux3: triple byte value */
                        int ticks = br.readBe16(); /* yes, really big-endian */
                        writeFlux(fluxmap, indexmarks, br, streamdelta, extrasclks + ticks);
                        extrasclks = 0;
                    } else if ((b >= 0x0e) && (b <= 0xff))
                    {
                        /* Flux1: single byte value */
                        writeFlux(fluxmap, indexmarks, br, streamdelta, extrasclks + b);
                        extrasclks = 0;
                    } else
                        error("unknown stream block byte 0x%02x at 0x%08x", b, (long) br.pos() - 1);
                }
            }
        }

        if (!br.eof())
            error("I/O error reading stream");
        return fluxmap;
    }

    private static void writeFlux(
            Fluxmap fluxmap,
            TreeSet<Integer> indexmarks,
            ByteReader br,
            int streamdelta,
            long sclk)
    {
        if (!indexmarks.isEmpty())
        {
            Integer nextindex = indexmarks.first();
            int nextindexpos = nextindex + streamdelta;
            if (br.pos() >= nextindexpos)
            {
                fluxmap.appendIndex();
                indexmarks.remove(nextindex);
            }
        }

        int ticks = (int) ((double) sclk * TICKS_PER_SCLK);
        fluxmap.appendInterval(ticks);
        fluxmap.appendPulse();
    }

    private static boolean hasSuffix(String haystack, String needle)
    {
        if (needle.length() > haystack.length())
            return false;

        return haystack.substring(haystack.length() - needle.length()).equals(needle);
    }

    private static void error(String format, Object... args)
    {
        throw new FluxEngineException(String.format(format, args));
    }
}