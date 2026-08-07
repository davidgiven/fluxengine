package com.cowlark.fluxengine.cli;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import java.util.Map;
import java.util.function.Supplier;

public interface Command
{
    ImmutableMap<String, Supplier<? extends Command>> ANALYSABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder().put(
                    "driveresponse", stub(
                            "driveresponse",
                            "Measures the drive's ability to read and write pulses.")).put(
                    "layout",
                    stub("layout", "Produces a visualisation of the track/sector layout.")).build();

    ImmutableMap<String, Supplier<? extends Command>> FLUXFILEABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("ls", stub("ls", "Lists the contents of a flux file."))
                    .put("rm", stub("rm", "Removes flux from a flux file."))
                    .put("cp", stub("cp", "Copies flux from one flux file to another."))
                    .build();

    ImmutableMap<String, Supplier<? extends Command>> TESTABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("bandwidth", TestBandwidthCommand::new)
                    .put("voltages", TestVoltagesCommand::new)
                    .build();

    ImmutableMap<String, Supplier<? extends Command>> COMMANDS =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("inspect", stub("inspect", "Low-level analysis and inspection of a disk."))
                    .put(
                            "analyse",
                            () -> new CommandGroup(ANALYSABLES, "Disk and drive analysis tools."))
                    .put("read", stub("read", "Reads a disk, producing a sector image."))
                    .put("write", stub("write", "Writes a sector image to a disk."))
                    .put(
                            "fluxfile",
                            () -> new CommandGroup(
                                    FLUXFILEABLES,
                                    "Flux file manipulation operations."))
                    .put("format", stub("format", "Format a disk and make a file system on it."))
                    .put(
                            "rawwrite", stub(
                                    "rawwrite",
                                    "Writes a flux file to a disk. Warning: you can't use this to" +
                                            " copy disks."))
                    .put(
                            "convert",
                            stub("convert", "Converts a flux file from one format to another."))
                    .put(
                            "getdiskinfo",
                            stub("getdiskinfo", "Read volume metadata off a disk (or image)."))
                    .put("ls", stub("ls", "Show files on disk (or image)."))
                    .put("mv", stub("mv", "Rename a file on a disk (or image)."))
                    .put("rm", stub("rm", "Deletes a file (or directory) off a disk (or image)."))
                    .put("getfile", stub("getfile", "Read a file off a disk (or image)."))
                    .put(
                            "getfileinfo",
                            stub("getfileinfo", "Read file metadata off a disk (or image)."))
                    .put("putfile", stub("putfile", "Write a file to disk (or image)."))
                    .put("mkdir", stub("mkdir", "Create a directory on disk (or image)."))
                    .put("rpm", RpmCommand::new)
                    .put("seek", SeekCommand::new)
                    .put("devices", DevicesCommand::new)
                    .put("test", () -> new CommandGroup(TESTABLES, "Various testing commands."))
                    .build();

    /* Consume arguments until we reach a real command, instantiate it, and
     * run it with the tail of the argv array. */
    static boolean dispatch(Map<String, Supplier<? extends Command>> commands,
                            ImmutableList<String> args)
    {
        for (int index = 0; index < args.size(); index++)
        {
            Supplier<? extends Command> supplier = commands.get(args.get(index));
            if (supplier != null)
            {
                supplier.get().run(ImmutableList.copyOf(args.subList(index + 1, args.size())));
                return true;
            }
        }

        return false;
    }

    static Supplier<? extends Command> stub(String name, String help)
    {
        return () -> new StubCommand(name, help);
    }

    String getHelp();

    void run(ImmutableList<String> args);

}
