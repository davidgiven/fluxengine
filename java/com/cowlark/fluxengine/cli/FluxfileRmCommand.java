package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.Flags;
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
 * Removes flux from a flux file, modelled after src/fe-fluxfilerm.cc.
 */
public class FluxfileRmCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> fluxFilename = StringFlag.builder()
            .setGroup(flags)
            .setName("--fluxfile")
            .setName("-f")
            .setHelpText("flux file to remove from")
            .build();
    private ValueFlag<String> tracksFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--tracks")
            .setName("-t")
            .setHelpText("tracks to remove")
            .build();

    @Override
    public String getHelp()
    {
        return "Removes flux from a flux file.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        Flags.parse(args, flags);
        if (!fluxFilename.isSet())
            throw new FluxEngineException("you must specify a filename with -f");

        System.out.println(fluxFilename.get() + ":");
        FluxFileProto f = Fl2FluxSource.loadFl2File(fluxFilename.get());

        boolean changed = false;
        FluxFileProto.Builder builder = f.toBuilder();
        for (CylinderHead location : Locations.parseCylinderHeadsString(tracksFlag.get()))
        {
            boolean found = false;
            for (int i = 0; i < builder.getTrackCount(); i++)
            {
                TrackFluxProto trackFlux = builder.getTrack(i);
                if ((trackFlux.getTrack() == location.cylinder()) &&
                        (trackFlux.getHead() == location.head()))
                {
                    System.out.println(
                            "  removing c" + location.cylinder() + "h" + location.head());
                    builder.removeTrack(i);
                    found = changed = true;
                    i--;
                }
            }

            if (!found)
                System.out.println("  location c" + location.cylinder() + "h" + location.head() +
                        " not found");
        }

        if (changed)
        {
            System.out.println("writing back file");
            Fl2FluxSink.saveFl2File(fluxFilename.get(), builder);
        } else
            System.out.println("file not modified");
    }
}
