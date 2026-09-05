package com.cowlark.fluxengine.fluxsink;

import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_INDEX;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * A flux sink which writes VCD (Value Change Dump) files, ported from
 * lib/fluxsink/vcdfluxsink.cc.
 */
public class VcdFluxSink extends FluxSink
{
    private final String directory;

    public VcdFluxSink(String directory)
    {
        this.directory = directory;
    }

    @Override
    public void addFlux(int track, int head, Fluxmap fluxmap)
    {
        try
        {
            Files.createDirectories(Path.of(directory));
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot create directory '" + directory + "'");
        }

        StringBuilder sb = new StringBuilder();
        sb.append("$timescale 1ns $end\n");
        sb.append("$var wire 1 i index $end\n");
        sb.append("$var wire 1 p pulse $end\n");
        sb.append("$upscope $end\n");
        sb.append("$enddefinitions $end\n");
        sb.append("$dumpvars 0i 0p $end\n");

        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        long timestamp = 0;
        long lasttimestamp = 0;
        while (!fmr.eof())
        {
            FluxmapReader.Event event = fmr.getNextEvent();
            if (fmr.eof())
                break;

            long newtimestamp = timestamp + event.ticks();
            if (newtimestamp != lasttimestamp)
            {
                sb.append("\n#");
                sb.append((long) ((lasttimestamp + 1) * NS_PER_TICK));
                sb.append(" 0i 0p\n");
                timestamp = newtimestamp;
                sb.append("#");
                sb.append((long) (timestamp * NS_PER_TICK));
                sb.append(" ");
            }

            if ((event.event() & F_BIT_PULSE) != 0)
                sb.append("1p ");
            if ((event.event() & F_BIT_INDEX) != 0)
                sb.append("1i ");

            lasttimestamp = timestamp;
        }
        sb.append("\n");

        String filename = String.format("%s/c%02d.h%01d.vcd", directory, track, head);
        try
        {
            Files.write(Path.of(filename), sb.toString().getBytes());
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open output file");
        }
    }
}
