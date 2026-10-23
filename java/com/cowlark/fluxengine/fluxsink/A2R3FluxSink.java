package com.cowlark.fluxengine.fluxsink;

import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.A2R3;
import java.nio.charset.StandardCharsets;
import java.nio.file.Path;
import java.time.Instant;
import java.util.ArrayList;
import java.util.List;
import org.indunet.fastproto.FastProto;

/* Writes raw captures as A2R3, using the default 62.5 ns timing resolution. */
public class A2R3FluxSink extends FluxSink
{
    private final String filename;
    private final ConfigProto config;
    private final Bytes captures = new Bytes();
    private final ByteWriter captureWriter = captures.writer();

    public A2R3FluxSink(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;

        /* Every RWCP chunk begins with its version, resolution and reserved bytes. */
        captureWriter.write(FastProto.encode(new A2R3.RawCaptureHeader()));
    }

    private static long toA2rTicks(long ticks)
    {
        return Math.round(ticks * NS_PER_TICK * 1000.0 / A2R3.DEFAULT_PS_PER_TICK);
    }

    @Override
    public void addFlux(int cylinder, int head, Fluxmap fluxmap)
    {
        if (fluxmap.bytes() == 0)
            return;

        /* Locations use quarter tracks for Apple II drives and cylinder/side pairs otherwise. */
        boolean apple2 = config.getDrive().getDriveType() ==
                com.cowlark.fluxengine.external.DriveType.DRIVETYPE_APPLE2;
        long location = apple2 ? cylinder : (long) cylinder * 2 + head;
        require(cylinder >= 0 && head >= 0 && head <= 1 && (!apple2 || head == 0)
                && location <= 0xffff, "capture location is out of range");

        /* Quantize absolute event times to avoid accumulating rounding errors between pulses. */
        Bytes data = new Bytes();
        ByteWriter dataWriter = data.writer();
        List<Long> indexes = new ArrayList<>();
        long totalTicks = 0;
        long lastPulse = 0;
        for (byte value : fluxmap.rawBytes().toByteArray())
        {
            totalTicks += value & 0x3f;
            long now = toA2rTicks(totalTicks);
            require(now <= 0xffffffffL, "capture duration exceeds the A2R3 timing range");
            if ((value & F_BIT_INDEX) != 0 && now != 0)
                indexes.add(now);
            if ((value & F_BIT_PULSE) != 0)
            {
                long interval = now - lastPulse;
                while (interval >= 255)
                {
                    dataWriter.write8(255);
                    interval -= 255;
                }
                dataWriter.write8((int) interval);
                lastPulse = now;
            }
        }

        /* Trailing extension bytes preserve silence without inventing a flux transition.
         * A2R cannot encode a final partial extension without implying another pulse. */
        long trailing = toA2rTicks(totalTicks) - lastPulse;
        while (trailing >= 255)
        {
            dataWriter.write8(255);
            trailing -= 255;
        }
        require(indexes.size() <= 255, "too many index signals in one capture");

        /* Classify by actual duration, using measured revolutions when index signals exist. */
        double periodNs = revolutionPeriodNs(indexes, fluxmap.durationNs());
        A2R3.Capture capture = new A2R3.Capture();
        capture.captureType = fluxmap.durationNs() < 2.5 * periodNs ? A2R3.TIMING : A2R3.XTIMING;
        capture.location = (int) location;
        capture.indexSignalCount = indexes.size();
        capture.indexSignals = indexes.stream().mapToLong(Long::longValue).toArray();
        captureWriter.write(FastProto.encode(capture));

        A2R3.FluxData payload = new A2R3.FluxData();
        payload.data = data.toByteArray();
        payload.dataSize = payload.data.length;
        captureWriter.write(FastProto.encode(payload));
    }

    private double revolutionPeriodNs(List<Long> indexes, double durationNs)
    {
        /* A full revolution spans one index interval, or a full set of hard sector intervals. */
        int sectors = Math.max(1, config.getDrive().getHardSectorCount());
        if (indexes.size() > sectors)
            return (indexes.get(sectors) - indexes.get(0)) *
                    (A2R3.DEFAULT_PS_PER_TICK / 1000.0);

        /* An explicitly synchronized capture begins at an omitted index signal. */
        if (config.getDrive().getSyncWithIndex() && indexes.size() >= sectors)
            return indexes.get(sectors - 1) * (A2R3.DEFAULT_PS_PER_TICK / 1000.0);

        /* Without measured rotation, use the configured period. Indexless image tracks
         * represent one revolution; otherwise use the conventional 300 RPM fallback. */
        double configuredNs = config.getDrive().getRotationalPeriodMs() * 1_000_000;
        if (configuredNs > 0)
            return configuredNs;
        return indexes.isEmpty() ? durationNs : 200_000_000;
    }

    private static void require(boolean condition, String message)
    {
        if (!condition)
            throw new FluxEngineException("cannot write A2R3: " + message);
    }

    private static void writeChunk(ByteWriter writer, int id, byte[] data)
    {
        A2R3.ChunkHeader header = new A2R3.ChunkHeader();
        header.id = Integer.toUnsignedLong(id);
        header.size = data.length;
        writer.write(FastProto.encode(header));
        writer.write(data);
    }

    @Override
    public void close()
    {
        /* INFO records the closest A2R drive geometry available from the configuration. */
        A2R3.Info info = new A2R3.Info();
        info.creator = String.format("%-32s", "FluxEngine");
        info.driveType = switch (config.getDrive().getDriveType())
        {
            case DRIVETYPE_APPLE2 -> A2R3.DRIVE_525_SS_40_QUARTER_STEP;
            case DRIVETYPE_40TRACK -> A2R3.DRIVE_525_DS_40;
            default -> A2R3.DRIVE_35_DS_80;
        };
        info.writeProtected = 1;
        info.synchronizedCapture = config.getDrive().getSyncWithIndex() ? 1 : 0;
        info.hardSectorCount = config.getDrive().getHardSectorCount();
        require(info.hardSectorCount >= 0 && info.hardSectorCount <= 255,
                "hard sector count is out of range");

        /* Terminate a copy of the captures so closing does not modify the buffered entries. */
        Bytes raw = new Bytes(captures.toByteArray());
        ByteWriter rawWriter = raw.writer();
        rawWriter.seekToEnd();
        rawWriter.write(FastProto.encode(new A2R3.EndMarker()));
        Bytes file = new Bytes();
        ByteWriter writer = file.writer();
        writer.write(FastProto.encode(new A2R3.FileHeader()));
        writeChunk(writer, A2R3.CHUNK_INFO, FastProto.encode(info));
        writeChunk(writer, A2R3.CHUNK_RWCP, raw.toByteArray());

        /* Keep the imaging timestamp as UTF-8 metadata following the raw captures. */
        writeChunk(writer, A2R3.CHUNK_META,
                ("image_date\t" + Instant.now() + "\n").getBytes(StandardCharsets.UTF_8));
        file.writeToFile(Path.of(filename));
    }
}
