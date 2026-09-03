package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;

public class VfsGetFileCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> pathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path")
            .setName("-p")
            .setHelpText("disk path to work on")
            .setDefaultValue("")
            .build();
    private ValueFlag<String> localFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--local")
            .setName("-l")
            .setHelpText("local filename to write to")
            .setDefaultValue("")
            .build();

    @Override
    public String getHelp()
    {
        return "Read a file off a disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        String pathString = pathFlag.get();
        if (pathString.isEmpty())
            throw new FluxEngineException("you must supply a filename to read");

        String outputFilename = localFlag.get();
        if (outputFilename.isEmpty())
        {
            VfsPath path = VfsPath.of(pathString);
            String name = path.getName();
            if (name == null)
                throw new FluxEngineException("you must supply a filename to read");
            outputFilename = name;
        }

        String finalOutputFilename = outputFilename;

        Filesystem.doWithFilesystem(
                config, fs -> {
                    Bytes data = fs.getFile(VfsPath.of(pathString));
                    data.writeToFile(finalOutputFilename);
                });
    }
}
