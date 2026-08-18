package com.cowlark.fluxengine.fluxsource;

import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;
import static com.cowlark.fluxengine.external.Scp.SCP_FLAG_96TPI;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.DriveType;
import com.cowlark.fluxengine.external.Scp;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * A flux source which reads an SCP flux file, ported from
 * lib/fluxsource/scpfluxsource.cc.
 */
public class ScpFluxSource extends TrivialFluxSource
{
    private final Bytes data;
    private final double resolution;
    private final int heads;
    private final int startTrack;
    private final int endTrack;
    private final int flags;
    private final int revolutions;
    private final int[] trackOffsets = new int[168];
    protected ConfigProto extraConfig;

    public ScpFluxSource(ScpFluxSourceProto config)
    {
        data = readFile(config.getFilename());

        ByteReader br = new ByteReader(data);
        byte[] fileId = br.read(3).toByteArray();
        if ((fileId[0] != 'S') || (fileId[1] != 'C') || (fileId[2] != 'P'))
            throw new FluxEngineException("input not a SCP file");

        br.read8(); /* version */
        br.read8(); /* type */
        revolutions = br.read8();
        startTrack = Scp.trackno(br.read8());
        endTrack = Scp.trackno(br.read8());
        flags = br.read8();
        int cellWidth = br.read8();
        heads = br.read8();
        int resolutionByte = br.read8();
        br.skip(4); /* checksum */

        for (int i = 0; i < 168; i++)
            trackOffsets[i] = br.readLe32();

        if ((cellWidth != 0) && (cellWidth != 16))
            throw new FluxEngineException("currently only 16-bit cells in SCP files are supported");

        resolution = 25.0 * (resolutionByte + 1);

        int startSide = (heads == 2) ? 1 : 0;
        int endSide = (heads == 1) ? 0 : 1;

        List<CylinderHead> chs = new ArrayList<>();
        for (int cylinder = startTrack; cylinder <= endTrack; cylinder++)
            for (int head = startSide; head <= endSide; head++)
                chs.add(new CylinderHead(cylinder, head));

        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder()
                .setDriveType((flags & SCP_FLAG_96TPI) != 0 ?
                        DriveType.DRIVETYPE_80TRACK :
                        DriveType.DRIVETYPE_40TRACK);
        builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(chs));
        extraConfig = builder.build();

        Logger.logf("SCP tracks %d-%d, heads %d-%d", startTrack, endTrack, startSide, endSide);
        Logger.logf("SCP sample resolution: %d ns", (int) resolution);
    }

    private static Bytes readFile(String filename)
    {
        try
        {
            return new Bytes(Files.readAllBytes(Path.of(filename)));
        } catch (IOException e)
        {
            throw new FluxEngineException(
                    "cannot open input file '" + filename + "': " + e.getMessage());
        }
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public Fluxmap readSingleFlux(FluxReadParameters parameters)
    {
        int strack = Scp.strackno(parameters.cylinder(), parameters.head());
        if (strack >= 168)
            return new Fluxmap();
        int offset = trackOffsets[strack];
        if (offset == 0)
            return new Fluxmap();

        ByteReader br = new ByteReader(data);
        br.seek(offset);
        byte[] trackId = br.read(3).toByteArray();
        if ((trackId[0] != 'T') || (trackId[1] != 'R') || (trackId[2] != 'K'))
            throw new FluxEngineException("corrupt SCP file");
        br.read8(); /* strack */

        int[] revsLength = new int[revolutions];
        int[] revsOffset = new int[revolutions];
        for (int revolution = 0; revolution < revolutions; revolution++)
        {
            br.skip(4); /* index */
            revsLength[revolution] = br.readLe32();
            revsOffset[revolution] = br.readLe32();
        }

        Fluxmap fluxmap = new Fluxmap();
        long pending = 0;
        for (int revolution = 0; revolution < revolutions; revolution++)
        {
            if (revolution != 0)
                fluxmap.appendIndex();

            int dataLength = revsLength[revolution];
            int dataOffset = revsOffset[revolution];

            ByteReader dbr = new ByteReader(data);
            dbr.seek(dataOffset + offset);
            for (int cell = 0; cell < dataLength; cell++)
            {
                int interval = dbr.readBe16();
                if (interval != 0)
                {
                    fluxmap.appendInterval((int) ((interval + pending) * resolution / NS_PER_TICK));
                    fluxmap.appendPulse();
                    pending = 0;
                } else
                    pending += 0x10000;
            }
        }

        return fluxmap;
    }
}
