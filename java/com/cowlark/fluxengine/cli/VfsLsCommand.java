package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

public class VfsLsCommand extends AbstractVfsCommand
{
    private FlagGroup flags = new FlagGroup(vfsFlags);
    private ValueFlag<String> pathFlag = StringFlag
            .builder()
            .setGroup(flags)
            .setName("--path")
            .setName("-p")
            .setHelpText("path to list")
            .setDefaultValue("/")
            .build();

    private static char fileTypeChar(Filesystem.FileType fileType)
    {
        switch (fileType)
        {
            case IS_FILE:
                return ' ';

            case IS_DIR:
                return 'D';

            default:
                return '?';
        }
    }

    @Override
    public String getHelp()
    {
        return "Show files on disk (or image).";
    }

    @Override
    public void run(ImmutableList<String> args) throws Exception
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        applyVfsFlags(builder);
        ConfigProto config = builder.build();

        Filesystem.doWithFilesystem(
                config, fs -> {
                    ImmutableMap<String, Dirent> files = fs.list(VfsPath.of(pathFlag.get()));

                    int maxlen = 0;
                    for (Dirent dirent : files.values())
                        maxlen = Math.max(maxlen, Utils.quoteString(dirent.filename()).length());

                    int total = 0;
                    for (Dirent dirent : files.values())
                    {
                        String ctime = dirent.attributes().getOrDefault("ctime", "");
                        System.out.printf(
                                "%c %-" + (maxlen + 2) + "s  %6d %4s %s%n",
                                fileTypeChar(dirent.fileType()),
                                Utils.quoteString(dirent.filename()),
                                dirent.length(),
                                dirent.mode(),
                                ctime);
                        total += dirent.length();
                    }
                    System.out.printf("(%d files, %d bytes)%n", files.size(), total);
                });
    }
}
