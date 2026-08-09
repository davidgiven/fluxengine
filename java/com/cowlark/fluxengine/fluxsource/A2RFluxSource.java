package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.DriveType;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.TreeMap;

/**
 * A flux source which reads an A2R flux file, ported from
 * lib/fluxsource/a2rfluxsource.cc.
 */
public class A2RFluxSource extends FluxSource
{
    private final TreeMap<CylinderHead, A2Rv2Flux> v2data = new TreeMap<>();
    private final A2rFluxSourceProto config;
    private final Bytes data;
    protected ConfigProto extraConfig;
    private int version;

    public A2RFluxSource(A2rFluxSourceProto config)
    {
        this.config = config;
        data = readFile(config.getFilename());
        ByteReader br = new ByteReader(data);

        switch (br.readBe32())
        {
            case 0x41325232:
            {
                version = 2;
                Bytes info = findChunk(new Bytes("INFO"));
                int disktype = info.getByte(33) & 0xff;
                DriveType driveType;
                if (disktype == 1)
                {
                    /* 5.25" with quarter stepping. */
                    driveType = DriveType.DRIVETYPE_APPLE2;
                } else
                {
                    /* 3.5". */
                    driveType = DriveType.DRIVETYPE_80TRACK;
                }

                Bytes stream = findChunk(new Bytes("STRM"));
                ByteReader bsr = new ByteReader(stream);
                for (; ; )
                {
                    int location = bsr.read8();
                    if (location == 0xff)
                        break;
                    CylinderHead key = (disktype == 1) ?
                            new CylinderHead(location, 0) :
                            new CylinderHead(location >> 1, location & 1);

                    bsr.skip(1);
                    int len = bsr.readLe32();
                    double index = (double) bsr.readLe32() * 125;
                    A2Rv2Flux entry = v2data.get(key);
                    if (entry == null)
                    {
                        entry = new A2Rv2Flux();
                        entry.index = index;
                        v2data.put(key, entry);
                    }

                    entry.flux.add(bsr.read(len));
                }

                List<CylinderHead> chs = new ArrayList<>(v2data.keySet());

                ConfigProto.Builder builder = ConfigProto.newBuilder();
                builder.getDriveBuilder().setDriveType(driveType);
                builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(chs));
                extraConfig = builder.build();
                break;
            }

            default:
                error("unsupported A2R version");
        }
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

    private static void error(String message)
    {
        throw new FluxEngineException(message);
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        switch (version)
        {
            case 2:
            {
                A2Rv2Flux entry =
                        v2data.get(new CylinderHead(parameters.cylinder(), parameters.head()));
                if (entry != null)
                    return new A2RFluxSourceIterator(entry.flux, entry.index);
                else
                    return new EmptyFluxSourceIterator();
            }

            default:
                error("unsupported A2R version");
                return null;
        }
    }

    @Override
    public void recalibrate()
    {
    }

    private Bytes findChunk(Bytes id)
    {
        long offset = 8;
        while (offset < data.size())
        {
            ByteReader br = new ByteReader(data);
            br.seek((int) offset);
            if (br.read(4).equals(id))
            {
                int size = br.readLe32();
                return br.read(size);
            }

            offset += (long) br.readLe32() + 8;
        }

        error("A2R file missing chunk");
        return null;
    }

    static class A2Rv2Flux
    {
        List<Bytes> flux = new ArrayList<>();
        double index;
    }
}