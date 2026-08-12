package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.core.flags.Flags.parse;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.external.FluxFileProto;
import com.cowlark.fluxengine.external.TrackFluxProto;
import com.cowlark.fluxengine.fluxsource.Fl2FluxSource;
import com.google.common.collect.ImmutableList;

/**
 * Lists the contents of a flux file, modelled after src/fe-fluxfilels.cc.
 */
public class FluxfileLsCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> fluxFilename = StringFlag.builder()
            .setGroup(flags)
            .setName("--fluxfile")
            .setName("-f")
            .setHelpText("flux file to show")
            .build();

    @Override
    public String getHelp()
    {
        return "Lists the contents of a flux file.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        parse(args, flags);
        if (!fluxFilename.isSet())
            throw new FluxEngineException("you must specify a filename with -f");

        System.out.println(fluxFilename.get() + ":");
        FluxFileProto f = Fl2FluxSource.loadFl2File(fluxFilename.get());

        String[] fields = {"version", "rotational_period_ms", "drive_type", "format_type"};
        for (String field : fields)
        {
            String value;
            switch (field)
            {
                case "version":
                    value = f.getVersion().name();
                    break;
                case "rotational_period_ms":
                    value = Double.toString(f.getRotationalPeriodMs());
                    break;
                case "drive_type":
                    value = f.getDriveType().name();
                    break;
                case "format_type":
                    value = f.getFormatType().name();
                    break;
                default:
                    throw new IllegalStateException();
            }
            System.out.println("  " + field + ": " + value);
        }

        for (TrackFluxProto trackFlux : f.getTrackList())
        {
            System.out.print(
                    "  flux for c" + trackFlux.getTrack() + "h" + trackFlux.getHead() + ":");

            boolean first = true;
            for (int i = 0; i < trackFlux.getFluxCount(); i++)
            {
                Fluxmap fluxmap = new Fluxmap(new Bytes(trackFlux.getFlux(i).toByteArray()));
                if (!first)
                    System.out.print(",");
                System.out.printf(" %.3fms", fluxmap.durationNs() / 1000000.0);
                first = false;
            }

            System.out.println();
        }
    }
}
