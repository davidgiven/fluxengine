package com.cowlark.fluxengine.fluxsink;

import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.A2R;
import com.cowlark.fluxengine.external.DriveType;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.time.ZoneOffset;
import java.time.ZonedDateTime;
import java.time.format.DateTimeFormatter;
import java.util.LinkedHashMap;
import java.util.Map;

/**
 * A flux sink which writes an A2R flux file, ported from
 * lib/fluxsink/a2rfluxsink.cc.
 */
public class A2RFluxSink extends FluxSink
{
    private static final String VERSION_STRING = String.format("%-32s", "FluxEngine");
    private final String filename;
    private final ConfigProto config;
    private final Bytes bytes = new Bytes(0);
    private final ByteWriter writer = bytes.writer();
    private final Bytes strmBytes = new Bytes(0);
    private final ByteWriter strmWriter = strmBytes.writer();
    private final Map<String, String> metadata = new LinkedHashMap<>();
    private int minHead;
    private int maxHead;
    private int minCylinder;
    private int maxCylinder;

    public A2RFluxSink(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;
        metadata.put(
                "image_date",
                DateTimeFormatter.ISO_INSTANT.format(ZonedDateTime.now(ZoneOffset.UTC)));
    }

    private static long ticksToA2r(long ticks)
    {
        return (long) (ticks * NS_PER_TICK / A2R.NS_PER_TICK);
    }

    private void writeChunkAndData(int chunkId, Bytes data)
    {
        writer.writeLe32(chunkId);
        writer.writeLe32(data.size());
        writer.write(data);
    }

    private void writeHeader()
    {
        writer.write(Bytes.of(
                A2R.FILEHEADER[0] & 0xff,
                A2R.FILEHEADER[1] & 0xff,
                A2R.FILEHEADER[2] & 0xff,
                A2R.FILEHEADER[3] & 0xff,
                A2R.FILEHEADER[4] & 0xff,
                A2R.FILEHEADER[5] & 0xff,
                A2R.FILEHEADER[6] & 0xff,
                A2R.FILEHEADER[7] & 0xff));
    }

    private void writeInfo()
    {
        Bytes info = new Bytes(0);
        ByteWriter infoWriter = info.writer();
        infoWriter.write8(A2R.INFO_CHUNK_VERSION);
        infoWriter.write(VERSION_STRING.getBytes());

        infoWriter.write8((config.getDrive().getDriveType() == DriveType.DRIVETYPE_APPLE2) ?
                A2R.DISK_525 :
                A2R.DISK_35);

        infoWriter.write8(1); /* write protected */
        infoWriter.write8(1); /* synchronized */
        writeChunkAndData(A2R.CHUNK_INFO, info);
    }

    private void writeMeta()
    {
        Bytes meta = new Bytes(0);
        ByteWriter metaWriter = meta.writer();
        for (Map.Entry<String, String> i : metadata.entrySet())
        {
            metaWriter.write(i.getKey().getBytes());
            metaWriter.write8('\t');
            metaWriter.write(i.getValue().getBytes());
            metaWriter.write8('\n');
        }
        writeChunkAndData(A2R.CHUNK_META, meta);
    }

    private void writeStream()
    {
        /* A STRM always ends with a 255, even though this could ALSO
         * indicate the first byte of a multi-byte sequence */
        strmWriter.write8(255);

        writeChunkAndData(A2R.CHUNK_STRM, strmBytes);
    }

    @Override
    public void addFlux(int cylinder, int head, Fluxmap fluxmap)
    {
        if (fluxmap.bytes() == 0)
        {
            return;
        }

        // Writing from an image (as opposed to from a floppy) will
        // contain exactly one revolution and no index events.
        FluxmapReader fmrCheck = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        fmrCheck.skipToEvent(F_BIT_INDEX);
        boolean isImage = fmrCheck.eof();

        // Write the flux data into its own Bytes
        Bytes trackBytes = new Bytes(0);
        ByteWriter trackWriter = trackBytes.writer();

        int[] revolutionHolder = {0};
        long[] loopPointHolder = {0};
        long[] totalTicksHolder = {0};

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        java.util.function.IntConsumer writeOneFlux = (ticks) -> {
            long value = ticksToA2r(ticks);
            while (value > 254)
            {
                trackWriter.write8(255);
                value -= 255;
            }
            trackWriter.write8((int) value);
        };

        java.util.function.IntConsumer writeFlux = (maxTicks) -> {
            long ticksSinceLastPulse = 0;

            while (!fmr.eof() && totalTicksHolder[0] < maxTicks)
            {
                FluxmapReader.Event event = fmr.getNextEvent();
                long ticks = event.ticks();

                ticksSinceLastPulse += ticks;
                totalTicksHolder[0] += ticks;

                if ((event.event() & F_BIT_PULSE) != 0)
                {
                    writeOneFlux.accept((int) ticksSinceLastPulse);
                    ticksSinceLastPulse = 0;
                }

                if ((event.event() & F_BIT_INDEX) != 0 && revolutionHolder[0] == 0)
                {
                    loopPointHolder[0] = totalTicksHolder[0];
                    revolutionHolder[0] += 1;
                }
            }
        };

        if (isImage)
        {
            // A timing stream with no index represents exactly one
            // revolution with no index. However, a2r nominally contains
            // 450 degress of rotation, 250ms at 300rpm.
            writeFlux.accept(Integer.MAX_VALUE);
            loopPointHolder[0] = totalTicksHolder[0];
            fmr.rewind();
            revolutionHolder[0] += 1;
            writeFlux.accept((int) (totalTicksHolder[0] * 5 / 4));
        } else
        {
            // We have an index, so this is a real read from a floppy
            // and should be "one revolution plus a bit"
            fmr.skipToEvent(F_BIT_INDEX);
            writeFlux.accept(Integer.MAX_VALUE);
        }

        if (config.getDrive().getDriveType() == DriveType.DRIVETYPE_APPLE2)
            strmWriter.write8(cylinder);
        else
            strmWriter.write8((cylinder << 1) | head);

        strmWriter.write8(A2R.TIMING);
        strmWriter.writeLe32(trackBytes.size());
        strmWriter.writeLe32((int) ticksToA2r(loopPointHolder[0]));
        strmWriter.write(trackBytes);
    }

    @Override
    public void close()
    {
        // FIXME: should use a passed-in DiskLayout object.
        DiskLayout diskLayout = DiskLayout.createDiskLayout(config);

        minCylinder = diskLayout.minPhysicalCylinder;
        maxCylinder = diskLayout.maxPhysicalCylinder;
        minHead = diskLayout.minPhysicalHead;
        maxHead = diskLayout.maxPhysicalHead;

        Logger.logf("A2R: writing A2R " + ((minHead == maxHead) ? "single sided" : "double sided") +
                " file containing " + (maxCylinder - minCylinder + 1) + " tracks...");

        writeHeader();
        writeInfo();
        writeStream();
        writeMeta();

        try
        {
            Files.write(Path.of(filename), bytes.toByteArray());
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open output file");
        }
    }
}
