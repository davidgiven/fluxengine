package com.cowlark.fluxengine.buildtools;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.OptionGroupProto;
import com.cowlark.fluxengine.config.OptionProto;
import com.cowlark.fluxengine.config.SupportStatus;
import com.cowlark.fluxengine.data.Formats;
import com.cowlark.fluxengine.vfs.FilesystemProto;
import com.google.common.collect.ImmutableList;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.List;
import java.util.TreeSet;

/**
 * Emits the Markdown index table of all disk formats, ported from
 * scripts/mkdocindex.cc. Format configurations are fetched with
 * {@link Formats#get(String)}.
 * <p>
 * Rather than writing to stdout, the table is spliced into the file named by
 * the second argument, replacing whatever lies between the
 * {@code <!-- FORMATSSTART -->} and {@code <!-- FORMATSEND -->} markers
 * (which are preserved).
 * <p>
 * Usage: MkDocIndex &lt;ignored&gt; &lt;output-file&gt;
 */
public final class MkDocIndex
{
    private static final String START_MARKER = "<!-- FORMATSSTART -->";
    private static final String END_MARKER = "<!-- FORMATSEND -->";

    private MkDocIndex()
    {
    }

    private static String supportStatus(SupportStatus status)
    {
        switch (status)
        {
            case DINOSAUR:
                return "🦖";

            case UNICORN:
                return "🦄";

            case UNSUPPORTED:
                return "";
        }

        return "";
    }

    private static void addFilesystem(TreeSet<String> filesystems, FilesystemProto fs)
    {
        if (fs.getType() != FilesystemProto.FilesystemType.NOT_SET)
            filesystems.add(fs.getType().name());
    }

    private static String filesystemsFor(ConfigProto config)
    {
        TreeSet<String> filesystems = new TreeSet<>();

        addFilesystem(filesystems, config.getFilesystem());
        for (OptionGroupProto group : config.getOptionGroupList())
            for (OptionProto option : group.getOptionList())
                addFilesystem(filesystems, option.getConfig().getFilesystem());
        for (OptionProto option : config.getOptionList())
            addFilesystem(filesystems, option.getConfig().getFilesystem());

        StringBuilder ss = new StringBuilder();
        for (String fs : filesystems)
            ss.append(fs).append(' ');
        return ss.toString();
    }

    /* Generates the index table itself (everything that goes between the
     * markers; the markers themselves live in the output file). */
    private static String buildIndex()
    {
        StringBuilder out = new StringBuilder();
        out.append("<!-- This section is automatically generated. " + "Do not edit. -->\n");
        out.append("\n");
        out.append("| Profile | Format | Read? | Write? | Filesystem? |\n");
        out.append("|:--------|:-------|:-----:|:------:|:------------|\n");

        /* The C++ iterates a std::map, so entries come out sorted by name. */
        for (String name : Formats.all().stream().sorted().collect(ImmutableList.toImmutableList()))
        {
            ConfigProto config = Formats.get(name);
            if (config == null)
                continue;
            if (config.getIsExtension())
                continue;

            out.append(String.format(
                    "| [`%s`](doc/disk-%1$s.md) | %s | %s | %s | %s |\n",
                    name,
                    config.getShortname() + ": " + config.getComment(),
                    supportStatus(config.getReadSupportStatus()),
                    supportStatus(config.getWriteSupportStatus()),
                    filesystemsFor(config)));
        }

        out.append("{: .datatable }\n");
        out.append("\n");
        return out.toString();
    }

    private static int indexOfLineContaining(List<String> lines, String marker)
    {
        for (int i = 0; i < lines.size(); i++)
            if (lines.get(i).contains(marker))
                return i;
        return -1;
    }

    private static void rewriteIndex(Path indexFilename) throws IOException
    {
        List<String> lines = Files.readAllLines(indexFilename);
        int start = indexOfLineContaining(lines, START_MARKER);
        int end = indexOfLineContaining(lines, END_MARKER);
        if ((start == -1) || (end == -1) || (end < start))
            throw new IOException(
                    "markers not found in " + indexFilename + ": need one line " + "containing " +
                            START_MARKER + " and one " + "containing " + END_MARKER);

        StringBuilder out = new StringBuilder();
        for (int i = 0; i <= start; i++)
            out.append(lines.get(i)).append('\n');
        out.append(buildIndex());
        for (int i = end; i < lines.size(); i++)
            out.append(lines.get(i)).append('\n');

        Files.writeString(indexFilename, out.toString());
    }

    public static void main(String[] args) throws IOException
    {
        rewriteIndex(Path.of(args[0]));
    }
}
