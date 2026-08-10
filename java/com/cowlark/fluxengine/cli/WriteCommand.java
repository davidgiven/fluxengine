package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.algorithms.WriteOperation;
import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.fluxsource.FluxSource;
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
    private boolean verify = true;
    private ActionFlag noVerifyFlag = ActionFlag.builder()
            .setGroup(flags)
            .setName("--no-verify")
            .setName("-n")
            .setHelpText("skip verification of write")
            .setVoidCallback(() -> verify = false)
            .build();

    @Override
    public String getHelp()
    {
        return "Writes a sector image to a disk.";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        var configProto = new ConfigBuilder().fromFlags(args, flags)
                .withImageReader(sourceImageFlag.get())
                .withFluxSink(destFluxFlag.get())
                .build();

        try (WriteOperation operation = new WriteOperation(configProto))
        {
            //                    Image image = operation.getImageReader().readImage();
            //
            //                    FluxSource verificationFluxSource = null;
            //                    if (configProto.hasDecoder() && operation.getFluxSinkFactory()
            //                    .isHardware() && verify)
            //                    {
            //                        verificationFluxSource = FluxSource.create(operation
            //                        .getVerificationFluxSource());
            //                    }
            //        ImageReader reader = operation.getImageReader();
            //        Image image = reader.readImage();
            //
            //        config = config.toBuilder().mergeFrom(reader.getExtraConfig()).build();
            //
            //        DiskLayout diskLayout = new DiskLayout(config);
            //        Encoder encoder = Arch.createEncoder(config);
            //        FluxSinkFactory fluxSinkFactory = FluxSinkFactory.create(config);
            //
            //        Decoder decoder = null;
            //        FluxSource verificationFluxSource = null;
            //        if (config.hasDecoder() && fluxSinkFactory.isHardware() && verify)
            //        {
            //            decoder = Arch.createDecoder(config);
            //            ConfigBuilder verifyBuilder = new ConfigBuilder().fromFlags(args, flags);
            //            verifyBuilder.withFluxSource(dest);
            //            verificationFluxSource = FluxSource.create(verifyBuilder.build());
            //        }
            //
            //        Writer.writeDiskCommand(
            //                config,
            //                diskLayout,
            //                image,
            //                encoder,
            //                fluxSinkFactory,
            //                decoder,
            //                verificationFluxSource);
        }
    }
}
