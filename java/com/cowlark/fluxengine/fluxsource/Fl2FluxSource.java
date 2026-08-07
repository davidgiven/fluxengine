package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.FluxFileVersion;
import com.cowlark.fluxengine.external.TrackFluxProto;
import com.google.protobuf.ByteString;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

/**
 * A flux source which reads an FL2 flux file, ported from
 * lib/fluxsource/fl2fluxsource.cc.
 */
public class Fl2FluxSource extends FluxSource
{
    private final FluxFileProto proto;
    protected ConfigProto extraConfig;

    public Fl2FluxSource(Fl2FluxSourceProto config)
    {
        proto = loadFl2File(config.getFilename());

        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setRotationalPeriodMs(proto.getRotationalPeriodMs());
        if (proto.hasDriveType())
            builder.getDriveBuilder().setDriveType(proto.getDriveType());

        List<String> tracks = new ArrayList<>();
        for (TrackFluxProto trackFlux : proto.getTrackList())
            tracks.add(String.format("c%dh%d", trackFlux.getTrack(), trackFlux.getHead()));
        builder.getDriveBuilder().setTracks(String.join(" ", tracks));

        extraConfig = builder.build();
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public FluxSourceIterator readFlux(int track, int head)
    {
        for (TrackFluxProto trackFlux : proto.getTrackList())
        {
            if (trackFlux.getTrack() == track && trackFlux.getHead() == head)
                return new Fl2FluxSourceIterator(trackFlux);
        }

        return new EmptyFluxSourceIterator();
    }

    @Override
    public void recalibrate()
    {
    }

    private static FluxFileProto loadFl2File(String filename)
    {
        Bytes data;
        try
        {
            data = new Bytes(Files.readAllBytes(Path.of(filename)));
        } catch (IOException e)
        {
            throw new FluxEngineException(
                    "cannot open input file '" + filename + "': " + e.getMessage());
        }

        if (data.size() >= 16 &&
                new String(data.slice(0, 16).toByteArray(), StandardCharsets.US_ASCII).equals(
                        "SQLite format 3"))
            throw new FluxEngineException(
                    "this flux file is too old; please use the upgrade-flux-file tool to upgrade " +
                            "it");

        FluxFileProto proto;
        try
        {
            proto = FluxFileProto.parseFrom(data.toByteArray());
        } catch (InvalidProtocolBufferException e)
        {
            throw new FluxEngineException("unable to read input file '" + filename + "'");
        }

        return upgradeFluxFile(proto);
    }

    private static FluxFileProto upgradeFluxFile(FluxFileProto proto)
    {
        if (proto.getVersion() == FluxFileVersion.VERSION_1)
        {
            /* Change a flux datastream with multiple segments separated by
             * F_DESYNC into multiple flux segments. */
            FluxFileProto.Builder builder = proto.toBuilder();
            for (int i = 0; i < proto.getTrackCount(); i++)
            {
                TrackFluxProto track = proto.getTrack(i);
                if (track.getFluxCount() != 0)
                {
                    Fluxmap oldFlux = new Fluxmap(new Bytes(track.getFlux(0).toByteArray()));
                    TrackFluxProto.Builder trackBuilder = track.toBuilder();
                    trackBuilder.clearFlux();
                    for (Fluxmap flux : oldFlux.split())
                        trackBuilder.addFlux(ByteString.copyFrom(flux.rawBytes().toByteArray()));
                    builder.setTrack(i, trackBuilder.build());
                }
            }
            builder.setVersion(FluxFileVersion.VERSION_2);
            proto = builder.build();
        }

        if (proto.getVersion().getNumber() > FluxFileVersion.VERSION_2.getNumber())
            throw new FluxEngineException("this is a version " + proto.getVersion().getNumber() +
                    " flux file, but this build of the client can only handle up to version " +
                    FluxFileVersion.VERSION_2.getNumber() + " --- please upgrade");
        return proto;
    }
}
