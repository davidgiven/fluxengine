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
import java.nio.file.Files;
import java.nio.file.Path;

public class VfsPutFileCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> pathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path")
            .setName("-p")
            .setHelpText("path to work on")
            .setDefaultValue("")
            .build();
    private ValueFlag<String> localFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--local")
            .setName("-l")
            .setHelpText("local filename to read from")
            .setDefaultValue("")
            .build();

    @Override
    public String getHelp()
    {
        return "Write a file to disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        String localString = localFlag.get();
        if (localString.isEmpty())
            throw new FluxEngineException("you must supply a local file to read from");

        String pathString = pathFlag.get();
        if (pathString.isEmpty())
            throw new FluxEngineException("you must supply a destination path to write to");

        Bytes data = new Bytes(Files.readAllBytes(Path.of(localString)));

        Filesystem.doWithFilesystem(
                config, fs -> {
                    fs.putFile(VfsPath.of(pathString), data);
                });
    }
}
