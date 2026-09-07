package com.cowlark.fluxengine.cli;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import java.util.Map;
import java.util.function.Supplier;

public interface Command
{
    ImmutableMap<String, Supplier<? extends Command>> ANALYSABLES = ImmutableMap
            .<String, Supplier<? extends Command>>builder()
            .put("driveresponse", AnalyseDriveResponse::new)
            .build();

    ImmutableMap<String, Supplier<? extends Command>> FLUXFILEABLES = ImmutableMap
            .<String, Supplier<? extends Command>>builder()
            .put("ls", FluxfileLsCommand::new)
            .put("rm", FluxfileRmCommand::new)
            .put("cp", FluxfileCpCommand::new)
            .build();

    ImmutableMap<String, Supplier<? extends Command>> TESTABLES = ImmutableMap
            .<String, Supplier<? extends Command>>builder()
            .put("bandwidth", TestBandwidthCommand::new)
            .put("voltages", TestVoltagesCommand::new)
            .build();


    ImmutableMap<String, Supplier<? extends Command>> VFSABLES = ImmutableMap
            .<String, Supplier<? extends Command>>builder()
            .put("ls", VfsLsCommand::new)
            .put("mv", VfsMvCommand::new)
            .put("rm", VfsRmCommand::new)
            .put("getfile", VfsGetFileCommand::new)
            .put("getfileinfo", VfsGetFileInfoCommand::new)
            .put("putfile", VfsPutFileCommand::new)
            .put("mkdir", VfsMkdirCommand::new)
            .put("getdiskinfo", VfsGetDiskInfoCommand::new)
            .put("format", VfsFormatCommand::new)
            .build();

    ImmutableMap<String, Supplier<? extends Command>> COMMANDS = ImmutableMap
            .<String, Supplier<? extends Command>>builder()
            .put(
                    "analyse",
                    () -> new CommandGroup(ANALYSABLES, "Disk and drive analysis tools."))
            .put("test", () -> new CommandGroup(TESTABLES, "Various testing commands."))
            .put(
                    "fluxfile",
                    () -> new CommandGroup(
                            FLUXFILEABLES,
                            "Flux file manipulation operations."))
            .put(
                    "vfs",
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

    static boolean dispatch(
            Map<String, Supplier<? extends Command>> commands,
            ImmutableList<String> args) throws Exception
    {
        Supplier<? extends Command> supplier = commands.get(args.getFirst());
        if (supplier != null)
        {
            supplier.get().run(ImmutableList.copyOf(args.subList(1, args.size())));
            return true;
        }

        return false;
    }

    String getHelp();

    void run(ImmutableList<String> args) throws Exception;

}
