package com.cowlark.fluxengine.fluxsink;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.TICK_FREQUENCY;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * A flux sink which writes Sun .au audio files, ported from
 * lib/fluxsink/aufluxsink.cc.
 */
public class AuFluxSink extends FluxSink
{
    private final String directory;
    private final boolean indexMarkers;

    public AuFluxSink(String directory, boolean indexMarkers)
    {
        this.directory = directory;
        this.indexMarkers = indexMarkers;
    }

    @Override
    public void addFlux(int track, int head, Fluxmap fluxmap)
    {
        Logger.logf("Warning: do not play these files, or you will break your " +
                "speakers and/or ears!");

        int totalTicks = fluxmap.ticks() + 2;
        int channels = indexMarkers ? 2 : 1;

        try
        {
            Files.createDirectories(Path.of(directory));
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot create directory '" + directory + "'");
        }

        Bytes data = new Bytes(totalTicks * channels);
        for (int i = 0; i < data.size(); i++)
            data.setByte(i, (byte) 0x80);

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        long timestamp = 0;
        while (!fmr.eof())
        {
            FluxmapReader.Event event = fmr.getNextEvent();
            if (fmr.eof())
                break;
            timestamp += event.ticks();

            if ((event.event() & F_BIT_PULSE) != 0)
                data.setByte((int) timestamp * channels, (byte) 0x7f);
            if (indexMarkers && (event.event() & F_BIT_INDEX) != 0)
                data.setByte((int) timestamp * channels + 1, (byte) 0x7f);
        }

        /* Write header */
        Bytes header = new Bytes(24);
        header.writer()
                .writeBe32(0x2e736e64)
                .writeBe32(24)
                .writeBe32(totalTicks * channels)
                .writeBe32(2) /* 8-bit PCM */.writeBe32(TICK_FREQUENCY)
                .writeBe32(channels); /* channels */

        String filename = String.format("%s/c%02d.h%01d.au", directory, track, head);
        try
        {
            Files.write(Path.of(filename), header.concat(data).toByteArray());
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open output file");
        }
    }
}
