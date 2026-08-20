package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.Kryoflux;
import java.io.File;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A flux source which reads raw Kryoflux stream files, ported from
 * lib/fluxsource/kryofluxfluxsource.cc.
 */
public class KryofluxFluxSource extends TrivialFluxSource
{
    private static final Pattern FILENAME_REGEX =
            Pattern.compile(".*[^0-9]([0-9]+)\\.([0-9]+)\\.raw");

    private final String path;
    protected ConfigProto extraConfig;

    public KryofluxFluxSource(KryofluxFluxSourceProto config)
    {
        path = config.getDirectory();

        List<CylinderHead> chs = new ArrayList<>();
        File[] files = new File(path).listFiles();
        if (files != null)
        {
            for (File f : files)
            {
                Matcher m = FILENAME_REGEX.matcher(f.getName());
                if (m.matches())
                    chs.add(new CylinderHead(
                            Integer.parseInt(m.group(1)),
                            Integer.parseInt(m.group(2))));
            }
        }

        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(chs));
        extraConfig = builder.build();
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public Fluxmap readSingleFlux(FluxReadParameters parameters)
    {
        return Kryoflux.readStream(path, parameters.cylinder(), parameters.head());
    }
}