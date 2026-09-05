package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.core.flags.Flags.parse;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.TrackFluxProto;
import com.cowlark.fluxengine.fluxsink.Fl2FluxSink;
import com.cowlark.fluxengine.fluxsource.Fl2FluxSource;
import com.google.common.collect.ImmutableList;

/**
 * Copies flux from one flux file to another, modelled after
 * src/fe-fluxfilecp.cc.
 */
public class FluxfileCpCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> inputFilenameFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--input")
            .setName("-i")
            .setHelpText("input flux file")
            .build();
    private ValueFlag<String> outputFilenameFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--output")
            .setName("-o")
            .setHelpText("output flux file (must exist)")
            .build();
    private ValueFlag<String> tracksFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--tracks")
            .setName("-t")
            .setHelpText("tracks to copy")
            .build();

    private static TrackFluxProto findTrack(FluxFileProto f, int cylinder, int head)
    {
        for (TrackFluxProto trackFlux : f.getTrackList())
            if ((trackFlux.getTrack() == cylinder) && (trackFlux.getHead() == head))
                return trackFlux;

        return null;
    }

    private static TrackFluxProto.Builder findOrMakeTrack(
            FluxFileProto.Builder f,
            int cylinder,
            int head)
    {
        for (TrackFluxProto.Builder trackFlux : f.getTrackBuilderList())
            if ((trackFlux.getTrack() == cylinder) && (trackFlux.getHead() == head))
                return trackFlux;

        TrackFluxProto.Builder tf = f.addTrackBuilder();
        tf.setTrack(cylinder);
        tf.setHead(head);
        return tf;
    }

    @Override
    public String getHelp()
    {
        return "Copies flux from one flux file to another.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        parse(args, flags);
        if (!inputFilenameFlag.isSet())
            throw new FluxEngineException("you must specify an input filename with -i");
        if (!outputFilenameFlag.isSet())
            throw new FluxEngineException("you must specify an output filename with -o");

        System.out.println(inputFilenameFlag.get() + " -> " + outputFilenameFlag.get() + ":");
        FluxFileProto inf = Fl2FluxSource.loadFl2File(inputFilenameFlag.get());
        FluxFileProto outf = Fl2FluxSource.loadFl2File(outputFilenameFlag.get());

        boolean changed = false;
        FluxFileProto.Builder outBuilder = outf.toBuilder();
        for (CylinderHead location : Locations.parseCylinderHeadsString(tracksFlag.get()))
        {
            TrackFluxProto intrack = findTrack(inf, location.cylinder(), location.head());
            if (intrack == null)
            {
                System.out.println("  location c" + location.cylinder() + "h" + location.head() +
                        " not found");
                continue;
            }

            TrackFluxProto.Builder outtrack =
                    findOrMakeTrack(outBuilder, location.cylinder(), location.head());
            System.out.println("  copying c" + location.cylinder() + "h" + location.head());
            for (int i = 0; i < intrack.getFluxCount(); i++)
                outtrack.addFlux(intrack.getFlux(i));
            changed = true;
        }

        if (changed)
        {
            System.out.println("writing back output file");
            Fl2FluxSink.saveFl2File(outputFilenameFlag.get(), outBuilder);
        } else
            System.out.println("output file not modified");
    }
}
