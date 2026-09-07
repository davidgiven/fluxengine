package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
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
    public void run(ImmutableList<String> args)
    {
        if (args.getFirst().startsWith("-"))
            throw new FluxEngineException("CLI verbs must come before all options, sorry!");

        if (!Command.dispatch(subcommands, args))
            Main.help(subcommands, "<command> <format> [<flags>...]");
    }
}
