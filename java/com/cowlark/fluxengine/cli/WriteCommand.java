package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.algorithms.Writer;
import com.cowlark.fluxengine.arch.Arch;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.imagereader.ImageReader;
import com.google.common.collect.ImmutableList;

/**
 * Write a sector image to a disk, modelled after src/fe-write.cc.
 */
public class WriteCommand implements Command
{
    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceImageFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--input")
            .setName("-i")
            .setHelpText("source image to read from")
            .build();
    private ValueFlag<String> destFluxFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--dest")
            .setName("-d")
            .setHelpText("flux destination to write to")
            .build();
    private ActionFlag noVerifyFlag = ActionFlag.builder()
            .setGroup(flags)
            .setName("--no-verify")
            .setName("-n")
            .setHelpText("skip verification of write")
            .setVoidCallback(() -> verify = false)
            .build();

    private boolean verify = true;

    @Override
    public String getHelp()
    {
        return "Writes a sector image to a disk.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        if (sourceImageFlag.isSet())
            builder.withImageReader(sourceImageFlag.get());

        String dest = destFluxFlag.isSet() ? destFluxFlag.get() : "drive:0";
        builder.withFluxSink(dest);
        ConfigProto config = builder.build();

        ImageReader reader = ImageReader.create(config);
        Image image = reader.readImage();

        config = config.toBuilder()
                .mergeFrom(reader.getExtraConfig())
                .build();

        DiskLayout diskLayout = new DiskLayout(config);
        Encoder encoder = Arch.createEncoder(config);
        FluxSinkFactory fluxSinkFactory = FluxSinkFactory.create(config);

        Decoder decoder = null;
        FluxSource verificationFluxSource = null;
        if (config.hasDecoder() && fluxSinkFactory.isHardware() && verify)
        {
            decoder = Arch.createDecoder(config);
            ConfigBuilder verifyBuilder = new ConfigBuilder().fromFlags(args, flags);
            verifyBuilder.withFluxSource(dest);
            verificationFluxSource = FluxSource.create(verifyBuilder.build());
        }

        Writer.writeDiskCommand(
                config,
                diskLayout,
                image,
                encoder,
                fluxSinkFactory,
                decoder,
                verificationFluxSource);
    }
}
