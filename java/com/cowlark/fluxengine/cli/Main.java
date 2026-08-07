package com.cowlark.fluxengine.cli;

import com.google.common.collect.ImmutableList;
import java.util.Map;
import java.util.function.Supplier;

/**
 * Command-line entry point, ported from src/fluxengine.cc. The command and
 * subcommand tables live here; main() consumes arguments until it reaches a
 * real command, instantiates it, and runs it with the tail of the argv array.
 */
public class Main
{

    private Main()
    {
    }

    public static void main(String[] args)
    {
        if (args.length == 0 || args[0].equals("--help"))
        {
            help(Command.COMMANDS, "<command> [<flags>...]");
            return;
        }

        if (!Command.dispatch(Command.COMMANDS, ImmutableList.copyOf(args)))
        {
            System.err.println("fluxengine: unrecognised command (try --help)");
            System.exit(1);
        }
    }

    static void help(Map<String, Supplier<? extends Command>> commands, String syntax)
    {
        System.out.printf("fluxengine: syntax: fluxengine %s\n", syntax);
        System.out.println("Try one of these commands:");
        for (Map.Entry<String, Supplier<? extends Command>> entry : commands.entrySet())
            System.out.printf("  %s: %s\n", entry.getKey(), entry.getValue().get().getHelp());
    }
}
