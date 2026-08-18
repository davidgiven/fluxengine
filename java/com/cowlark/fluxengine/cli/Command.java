package com.cowlark.fluxengine.cli;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import java.util.Map;
import java.util.function.Supplier;

public interface Command
{
    ImmutableMap<String, Supplier<? extends Command>> ANALYSABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("driveresponse",
                            stub("driveresponse",
                                    "Measures the drive's ability to read and write pulses."))
                    .put("layout",
                            stub("layout", "Produces a visualisation of the track/sector layout."))
                    .build();

    ImmutableMap<String, Supplier<? extends Command>> FLUXFILEABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("ls", FluxfileLsCommand::new)
                    .put("rm", FluxfileRmCommand::new)
                    .put("cp", FluxfileCpCommand::new)
                    .build();

    ImmutableMap<String, Supplier<? extends Command>> TESTABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("bandwidth", TestBandwidthCommand::new)
                    .put("voltages", TestVoltagesCommand::new)
                    .build();


    ImmutableMap<String, Supplier<? extends Command>> VFSABLES =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("ls", stub("ls", "Show files on disk (or image)."))
                    .put("mv", stub("mv", "Rename a file on a disk (or image)."))
                    .put("rm", stub("rm", "Deletes a file (or directory) off a disk (or image)."))
                    .put("getfile", stub("getfile", "Read a file off a disk (or image)."))
                    .put("getfileinfo",
                            stub("getfileinfo", "Read file metadata off a disk (or image)."))
                    .put("putfile", stub("putfile", "Write a file to disk (or image)."))
                    .put("mkdir", stub("mkdir", "Create a directory on disk (or image)."))
                    .put("getdiskinfo",
                            stub("getdiskinfo", "Read volume metadata off a disk (or image)."))
                    .put("format", stub("format", "Format a disk and make a file system on it."))
                    .build();

    ImmutableMap<String, Supplier<? extends Command>> COMMANDS =
            ImmutableMap.<String, Supplier<? extends Command>>builder()
                    .put("analyse",
                            () -> new CommandGroup(ANALYSABLES, "Disk and drive analysis tools."))
                    .put("test", () -> new CommandGroup(TESTABLES, "Various testing commands."))
                    .put("fluxfile",
                            () -> new CommandGroup(FLUXFILEABLES,
                                    "Flux file manipulation operations."))
                    .put("vfs",
                            () -> new CommandGroup(VFSABLES, "File system manipulation commands."))
                    .put("read", ReadCommand::new)
                    .put("write", WriteCommand::new)
                    .put("rawwrite", RawwriteCommand::new)
                    .put("convert", ConvertCommand::new)
                    .put("rpm", RpmCommand::new)
                    .put("seek", SeekCommand::new)
                    .put("devices", DevicesCommand::new)
                    .put("inspect", InspectCommand::new)
                    .put("gui", GuiCommand::new)
                    .build();

    /* Consume arguments until we reach a real command, instantiate it, and
     * run it with the tail of the argv array. */
    static boolean dispatch(
            Map<String, Supplier<? extends Command>> commands,
            ImmutableList<String> args)
    {
        for (int index = 0; index < args.size(); index++)
        {
            Supplier<? extends Command> supplier = commands.get(args.get(index));
            if (supplier != null)
            {
                try
                {
                    supplier.get().run(ImmutableList.copyOf(args.subList(index + 1, args.size())));
                } catch (Exception e)
                {
                    throw new RuntimeException(e);
                }
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

    void run(ImmutableList<String> args) throws Exception;

}
