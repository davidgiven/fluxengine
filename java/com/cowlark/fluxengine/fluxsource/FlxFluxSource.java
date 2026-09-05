package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.external.Flx;
import java.io.File;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/**
 * A flux source which reads FLX flux stream files, ported from
 * lib/fluxsource/flxfluxsource.cc.
 */
public class FlxFluxSource extends TrivialFluxSource
{
    private static final Pattern FILENAME_REGEX = Pattern.compile("@TR([0-9]+)S([0-9]+)@\\.FLX");

    private final String path;
    protected ConfigProto extraConfig;

    public FlxFluxSource(FlxFluxSourceProto config)
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
                            Integer.parseInt(m.group(2)) - 1));
            }
        }

        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(chs));
        extraConfig = builder.build();
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
        String path = String.format(
                "%s/@TR%02dS%d@.FLX",
                this.path,
                parameters.cylinder(),
                parameters.head() + 1);
        if (!Files.exists(Path.of(path)))
            return new Fluxmap();
        Logger.logf("FLX: reading %s", path);
        return Flx.readFlxBytes(readFile(path));
    }
}
