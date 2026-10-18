package com.cowlark.fluxengine.fluxsource;

import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.DriveProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.A2R3;
import com.cowlark.fluxengine.external.DriveType;
import com.google.common.collect.ImmutableList;
import org.indunet.fastproto.FastProto;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Iterator;
import java.util.List;
import java.util.TreeMap;

/* Reads raw A2R3 captures. SLVD, META and unknown chunks are skipped. */
public class A2R3FluxSource extends FluxSource
{
    private final TreeMap<CylinderHead, List<Capture>> tracks = new TreeMap<>();
    private final ConfigProto extraConfig;

    public A2R3FluxSource(A2rFluxSourceProto config)
    {
        /* Load the file and validate its format signature before reading chunks. */
        ByteBuffer file = Bytes
                .readFromFile(Path.of(config.getFilename()))
                .toByteBuffer()
                .order(ByteOrder.LITTLE_ENDIAN);
        FastProto.decode(take(file, 8), A2R3.FileHeader.class);

        /* Read bounded chunk bodies, ignoring chunks without raw capture data. */
        A2R3.Info info = null;
        while (file.hasRemaining())
        {
            A2R3.ChunkHeader chunk = FastProto.decode(take(file, 8), A2R3.ChunkHeader.class);
            require(chunk.size <= file.remaining(), "truncated chunk");

            /* Limit each decoder to this chunk and advance past unhandled chunks. */
            ByteBuffer body = file.slice().order(ByteOrder.LITTLE_ENDIAN);
            body.limit((int) chunk.size);
            file.position(file.position() + (int) chunk.size);

            /* INFO supplies the drive geometry needed to interpret capture locations. */
            require(info != null || chunk.id == A2R3.CHUNK_INFO, "INFO must be first");
            if (chunk.id == A2R3.CHUNK_INFO)
            {
                require(info == null, "duplicate INFO chunk");
                info = FastProto.decode(take(body, 37), A2R3.Info.class);
            } else if (chunk.id == A2R3.CHUNK_RWCP)
                readCaptures(body, info);
        }

        /* Translate the recorded drive geometry into FluxEngine configuration. */
        require(info != null, "missing INFO chunk");
        DriveType driveType = switch (info.driveType)
        {
            case A2R3.DRIVE_525_SS_40_QUARTER_STEP -> DriveType.DRIVETYPE_APPLE2;
            case A2R3.DRIVE_525_DS_40, A2R3.DRIVE_3_DS_40 -> DriveType.DRIVETYPE_40TRACK;
            case A2R3.DRIVE_35_DS_80_APPLE_CLV, A2R3.DRIVE_525_DS_80, A2R3.DRIVE_35_DS_80,
                 A2R3.DRIVE_3_DS_80 -> DriveType.DRIVETYPE_80TRACK;
            case A2R3.DRIVE_8_DS -> DriveType.DRIVETYPE_UNKNOWN;
            default -> throw new FluxEngineException("unsupported A2R3 drive type");
        };

        /* Restrict subsequent reads to the locations present in the raw captures. */
        extraConfig = ConfigProto
                .newBuilder()
                .setDrive(DriveProto
                        .newBuilder()
                        .setDriveType(driveType)
                        .setHardSectorCount(info.hardSectorCount)
                        .setTracks(Locations.convertCylinderHeadsToString(ImmutableList.copyOf(
                                tracks.keySet()))))
                .build();
    }

    private void readCaptures(ByteBuffer body, A2R3.Info info)
    {
        /* Resolution belongs to the RWCP chunk and can differ between chunks. */
        A2R3.RawCaptureHeader header =
                FastProto.decode(take(body, 16), A2R3.RawCaptureHeader.class);
        require(header.resolution > 0, "zero resolution");

        /* Capture entries are packed together and end with a single marker byte. */
        while (body.hasRemaining())
        {
            if ((body.get(body.position()) & 0xff) == A2R3.END_MARK)
            {
                FastProto.decode(take(body, 1), A2R3.EndMarker.class);
                require(!body.hasRemaining(), "data after capture terminator");
                return;
            }

            /* Decode the capture header together with its variable-length index array. */
            require(body.remaining() >= 5, "truncated capture header");
            int indexCount = body.get(body.position() + 4) & 0xff;
            A2R3.Capture capture =
                    FastProto.decode(take(body, 5 + 4 * indexCount), A2R3.Capture.class);

            /* Bound the payload before handing its recorded length to FastProto. */
            require(body.remaining() >= 4, "missing capture size");
            long size = Integer.toUnsignedLong(body.getInt(body.position()));
            require(size <= body.remaining() - 4, "truncated capture data");
            A2R3.FluxData flux = FastProto.decode(take(body, 4 + (int) size), A2R3.FluxData.class);

            /* Validate the capture type and chronological order of its index signals. */
            require(
                    capture.captureType == A2R3.TIMING || capture.captureType == A2R3.XTIMING ||
                            capture.captureType == A2R3.BITS, "unknown capture type");
            long previous = 0;
            for (long index : capture.indexSignals)
            {
                require(index >= previous, "index signals out of order");
                previous = index;
            }

            /* Retain each capture separately at its physical cylinder and head. */
            CylinderHead location = info.driveType == A2R3.DRIVE_525_SS_40_QUARTER_STEP ?
                    new CylinderHead(capture.location, 0) :
                    new CylinderHead(capture.location >> 1, capture.location & 1);
            tracks
                    .computeIfAbsent(location, key -> new ArrayList<>())
                    .add(new Capture(capture, flux.data, header.resolution));
        }

        /* Reaching the chunk boundary without an end marker indicates incomplete data. */
        throw new FluxEngineException("invalid A2R3: missing capture terminator");
    }

    private static byte[] take(ByteBuffer buffer, int size)
    {
        require(size >= 0 && size <= buffer.remaining(), "truncated structure");
        byte[] bytes = new byte[size];
        buffer.get(bytes);
        return bytes;
    }

    private static void require(boolean condition, String message)
    {
        if (!condition)
            throw new FluxEngineException("invalid A2R3: " + message);
    }

    @Override
    public ConfigProto getExtraConfig()
    {
        return extraConfig;
    }

    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        List<Capture> captures =
                tracks.get(new CylinderHead(parameters.cylinder(), parameters.head()));
        if (captures == null)
            return new EmptyFluxSourceIterator();

        /* Decode captures lazily, with independent iteration for each read request. */
        Iterator<Capture> iterator = captures.iterator();
        return new FluxSourceIterator()
        {
            @Override
            public boolean hasNext()
            {
                return iterator.hasNext();
            }

            @Override
            public Fluxmap next()
            {
                return iterator.next().decode();
            }
        };
    }

    private record Capture(A2R3.Capture header, byte[] data, long resolution)
    {
        Fluxmap decode()
        {
            /* Merge flux transitions and recorded index signals onto one timeline. */
            Fluxmap map = new Fluxmap();
            Timeline timeline = new Timeline(map, header.indexSignals, resolution);
            long timePs = 0;

            /* Bits have a fixed clock; timing captures use the chunk resolution. */
            switch (header.captureType)
            {
                case A2R3.BITS ->
                {
                    /* Legacy bits are MSB first, normalized to 4 microseconds per bit. */
                    for (byte value : data)
                    {
                        for (int mask = 0x80; mask != 0; mask >>= 1)
                        {
                            timePs += 4_000_000;
                            timeline.advance(timePs);
                            if ((value & mask) != 0)
                                map.appendPulse();
                        }
                    }
                }

                case A2R3.TIMING, A2R3.XTIMING ->
                {
                    /* Both timing types use the recorded length, regardless of revolution count. */
                    long pending = 0;
                    for (byte value : data)
                    {
                        int ticks = value & 0xff;
                        pending += ticks;
                        if (ticks != 255)
                        {
                            timePs += pending * resolution;
                            timeline.advance(timePs);
                            map.appendPulse();
                            pending = 0;
                        }
                    }

                    /* Preserve elapsed time when the capture ends with extension bytes. */
                    if (pending != 0)
                        timeline.advance(timePs + pending * resolution);
                }
                default -> throw new FluxEngineException("invalid A2R3: unknown capture type");
            }
            return map;
        }
    }

    /* Round absolute event times so fractional FluxEngine ticks do not accumulate drift. */
    private static class Timeline
    {
        private final Fluxmap map;
        private final long[] indexes;
        private final long resolution;
        private int nextIndex;

        Timeline(Fluxmap map, long[] indexes, long resolution)
        {
            this.map = map;
            this.indexes = indexes;
            this.resolution = resolution;
        }

        void advance(long timePs)
        {
            /* Emit index events before the flux event at the requested time. */
            while (nextIndex < indexes.length && indexes[nextIndex] * resolution <= timePs)
            {
                appendTo(indexes[nextIndex++] * resolution);
                map.appendIndex();
            }

            /* Finish the interval at the flux event or the end of the capture. */
            appendTo(timePs);
        }

        private void appendTo(long timePs)
        {
            double ticks = timePs / (NS_PER_TICK * 1000.0);
            require(ticks <= Integer.MAX_VALUE, "capture is too long");
            int delta = (int) ticks - map.ticks();
            if (delta != 0)
                map.appendInterval(delta);
        }
    }
}
