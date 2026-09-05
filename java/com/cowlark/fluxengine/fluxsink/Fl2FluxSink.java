package com.cowlark.fluxengine.fluxsink;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.FluxFileVersion;
import com.cowlark.fluxengine.external.FluxMagic;
import com.cowlark.fluxengine.external.TrackFluxProto;
import com.google.protobuf.ByteString;
import org.apache.commons.lang3.tuple.Pair;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;

/**
 * A flux sink which writes an FL2 flux file, ported from
 * lib/fluxsink/fl2fluxsink.cc.
 */
public class Fl2FluxSink extends FluxSink
{
    private final String filename;
    private final ConfigProto config;
    private final Map<Pair<Integer, Integer>, List<Bytes>> data = new HashMap<>();

    public Fl2FluxSink(String filename, ConfigProto config)
    {
        this.filename = filename;
        this.config = config;

        try
        {
            Path path = Path.of(filename);
            Files.write(path, new byte[0]);
            Files.delete(path);
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open output file");
        }
    }

    public static void saveFl2File(String filename, FluxFileProto.Builder proto)
    {
        proto.setMagic(FluxMagic.MAGIC.getNumber());
        proto.setVersion(FluxFileVersion.VERSION_2);

        try
        {
            Files.write(Path.of(filename), proto.build().toByteArray());
        } catch (IOException e)
        {
            throw new FluxEngineException("unable to write output file '" + filename + "'");
        }
    }

    @Override
    public void addFlux(int track, int head, Fluxmap fluxmap)
    {
        data.computeIfAbsent(Pair.of(track, head), k -> new ArrayList<>()).add(fluxmap.rawBytes());
    }

    @Override
    public void close()
    {
        Logger.logf("FL2: writing " + filename);

        FluxFileProto.Builder proto = FluxFileProto.newBuilder();
        for (Map.Entry<Pair<Integer, Integer>, List<Bytes>> e : data.entrySet())
        {
            TrackFluxProto.Builder track = TrackFluxProto.newBuilder();
            track.setTrack(e.getKey().getLeft());
            track.setHead(e.getKey().getRight());
            for (Bytes fluxBytes : e.getValue())
                track.addFlux(ByteString.copyFrom(fluxBytes.toByteArray()));
            proto.addTrack(track);
        }

        proto.setRotationalPeriodMs(config.getDrive().getRotationalPeriodMs());
        proto.setDriveType(config.getDrive().getDriveType());
        proto.setFormatType(config.getLayout().getFormatType());

        saveFl2File(filename, proto);
    }
}
