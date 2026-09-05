package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.flags.BoolFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import org.apache.commons.lang3.function.Consumers;

public abstract class AbstractVfsCommand implements Command
{
    protected FlagGroup vfsFlags = new FlagGroup();
    protected ValueFlag<String> imageFlag = StringFlag
            .builder()
            .setGroup(vfsFlags)
            .setName("--image")
            .setName("-i")
            .setHelpText("image to work on")
            .build();
    protected ValueFlag<String> fluxFlag = StringFlag
            .builder()
            .setGroup(vfsFlags)
            .setName("--flux")
            .setName("-f")
            .setHelpText("flux source/sink to work on")
            .build();
    protected ValueFlag<Boolean> loggingFlag = BoolFlag
            .builder()
            .setGroup(vfsFlags)
            .setName("--logging")
            .setHelpText("enable verbose logging")
            .setDefaultValue(false)
            .build();

    protected void applyVfsFlags(ConfigBuilder builder)
    {
        if (imageFlag.isSet())
        {
            builder.withImageReader(imageFlag.get());
            builder.withImageWriter(imageFlag.get());
        }
        if (fluxFlag.isSet())
        {
            builder.withFluxSource(fluxFlag.get());
            builder.withFluxSink(fluxFlag.get());
        }

        if (!loggingFlag.get())
        {
            Logger.setLogger(Consumers.nop());
            System.setProperty("org.slf4j.simpleLogger.defaultLogLevel", "ERROR");
        }
    }
}
