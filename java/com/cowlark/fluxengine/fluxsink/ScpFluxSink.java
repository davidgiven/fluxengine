package com.cowlark.fluxengine.fluxsink;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;
import static com.cowlark.fluxengine.external.Scp.SCP_FLAG_96TPI;
import static com.cowlark.fluxengine.external.Scp.SCP_FLAG_INDEXED;
import static com.cowlark.fluxengine.external.Scp.SCP_HEADER_SIZE;
import static com.cowlark.fluxengine.external.Scp.SCP_TRACK_SIZE;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.DriveType;
import com.cowlark.fluxengine.external.Scp;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * A flux sink which writes an SCP flux file, ported from
 * lib/fluxsink/scpfluxsink.cc.
 */
public class ScpFluxSink extends FluxSink
{
    private final String filename;
    private final int typeByte;
    private final boolean alignWithIndex;
    private final ConfigProto config;
    /* The 688-byte file header. */
    private final byte[] fileheader = new byte[SCP_HEADER_SIZE];
    private final Bytes trackdata = new Bytes(0);

    public ScpFluxSink(String filename, int typeByte, boolean alignWithIndex, ConfigProto config)
    {
        this.filename = filename;
        this.typeByte = typeByte;
        this.alignWithIndex = alignWithIndex;
        this.config = config;

        // FIXME: should use a passed-in DiskLayout object.
        DiskLayout diskLayout = DiskLayout.createDiskLayout(config);
        int minCylinder = diskLayout.minPhysicalCylinder;
        int maxCylinder = diskLayout.maxPhysicalCylinder;
        int minHead = diskLayout.minPhysicalHead;
        int maxHead = diskLayout.maxPhysicalHead;

        fileheader[0] = 'S';
        fileheader[1] = 'C';
        fileheader[2] = 'P';
        fileheader[3] = 0x18; /* Version 1.8 of the spec */
        fileheader[4] = (byte) typeByte;
        fileheader[6] = (byte) Scp.strackno(minCylinder, minHead);
        fileheader[7] = (byte) Scp.strackno(maxCylinder, maxHead);
        int flags = SCP_FLAG_INDEXED;
        if (config.getDrive().getDriveType() == DriveType.DRIVETYPE_APPLE2)
            throw new FluxEngineException("you can't write Apple II flux images to SCP files yet");
        if (config.getDrive().getDriveType() != DriveType.DRIVETYPE_40TRACK)
            flags |= SCP_FLAG_96TPI;
        fileheader[8] = (byte) flags;
        fileheader[9] = 0; /* cell width */
        if ((minHead == 0) && (maxHead == 0))
            fileheader[10] = 1;
        else if ((minHead == 1) && (maxHead == 1))
            fileheader[10] = 2;
        else
            fileheader[10] = 0;

        Logger.logf("SCP: writing " + (((flags & SCP_FLAG_96TPI) != 0) ? 96 : 48) + " tpi " +
                ((minHead == maxHead) ? "single sided" : "double sided") + " file containing " +
                (fileheader[7] - fileheader[6] + 1) + " tracks");
    }

    private static void writeLe32(byte[] dest, int offset, int v)
    {
        dest[offset] = (byte) v;
        dest[offset + 1] = (byte) (v >> 8);
        dest[offset + 2] = (byte) (v >> 16);
        dest[offset + 3] = (byte) (v >> 24);
    }

    private static int appendChecksum(int checksum, Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        while (!br.eof())
            checksum += br.read8();
        return checksum;
    }

    @Override
    public void addFlux(int track, int head, Fluxmap fluxmap)
    {
        ByteWriter trackdataWriter = trackdata.writer();
        trackdataWriter.seekToEnd();
        int strack = Scp.strackno(track, head);

        if (strack >= 168)
        {
            Logger.logf("SCP: cannot write track " + track + " head " + head +
                    ", there are not enough Track Data Headers.");
            return;
        }
        /* ScpTrack: 'TRK' id, strack, then 5 revolution records. */
        byte[] trackHeader = new byte[SCP_TRACK_SIZE];
        trackHeader[0] = 'T';
        trackHeader[1] = 'R';
        trackHeader[2] = 'K';
        trackHeader[3] = (byte) strack;

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        Bytes fluxdata = new Bytes(0);
        ByteWriter fluxdataWriter = fluxdata.writer();

        int revolution = -1; /* -1 indicates that we are before the first index pulse */
        if (alignWithIndex)
        {
            fmr.skipToEvent(F_BIT_INDEX);
            revolution = 0;
        }
        long revTicks = 0;
        long totalTicks = 0;
        long ticksSinceLastPulse = 0;
        int startOffset = 0;
        while (revolution < 5)
        {
            FluxmapReader.Event event = fmr.getNextEvent();
            long ticks = event.ticks();

            ticksSinceLastPulse += ticks;
            totalTicks += ticks;
            revTicks += ticks;

            /* if we haven't output any revolutions yet by the end of the
             * track, assume that the whole track is one rev also discard
             * any duplicate index pulses */
            if (((fmr.eof() && revolution <= 0) ||
                    (((event.event() & F_BIT_INDEX) != 0) && revTicks > 0)))
            {
                if (fmr.eof() && revolution == -1)
                    revolution = 0;
                if (revolution >= 0)
                {
                    int revOffset = 4 + revolution * 12;
                    writeLe32(trackHeader, revOffset + 8, startOffset + SCP_TRACK_SIZE);
                    writeLe32(trackHeader, revOffset + 4, (fluxdataWriter.pos() - startOffset) / 2);
                    writeLe32(trackHeader, revOffset, (int) (revTicks * NS_PER_TICK / 25));
                }
                revolution++;
                revTicks = 0;
                startOffset = fluxdataWriter.pos();
            }
            if (fmr.eof())
                break;

            if ((event.event() & F_BIT_PULSE) != 0)
            {
                long t = (long) (ticksSinceLastPulse * NS_PER_TICK / 25);
                while (t >= 0x10000)
                {
                    fluxdataWriter.writeBe16(0);
                    t -= 0x10000;
                }
                fluxdataWriter.writeBe16((int) t);
                ticksSinceLastPulse = 0;
            }
        }

        fileheader[5] = (byte) revolution;
        writeLe32(fileheader, 16 + strack * 4, trackdataWriter.pos() + SCP_HEADER_SIZE);
        trackdataWriter.write(trackHeader);
        trackdataWriter.write(fluxdata);
    }

    @Override
    public void close()
    {
        int checksum = 0;
        checksum = appendChecksum(
                checksum,
                new Bytes(java.util.Arrays.copyOfRange(fileheader, 0x10, fileheader.length)));
        checksum = appendChecksum(checksum, trackdata);
        writeLe32(fileheader, 12, checksum);

        Logger.logf("SCP: writing output file");
        Bytes out = new Bytes(fileheader).concat(trackdata);
        try
        {
            Files.write(Path.of(filename), out.toByteArray());
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open output file");
        }
    }
}
