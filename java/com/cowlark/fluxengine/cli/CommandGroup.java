package com.cowlark.fluxengine.cli;

import java.util.Map;
import java.util.function.Supplier;

/**
 * A command which dispatches to a table of subcommands, modelled on the
 * mainExtended() helper in src/fluxengine.cc.
 */
public class CommandGroup implements Command
{
    private final Map<String, Supplier<? extends Command>> subcommands;
    private final String help;

    public CommandGroup(Map<String, Supplier<? extends Command>> subcommands, String help)
    {
        this.subcommands = subcommands;
        this.help = help;
    }

    @Override
    public String getHelp()
    {
        return help;
    }

    @Override
    public void run(String[] args)
    {
        if (!Command.dispatch(subcommands, args))
            Main.help(subcommands, "<command> <format> [<flags>...]");
    }
}
