package com.cowlark.fluxengine.cli;

import picocli.CommandLine;

public class Main
{
    public static void main(String[] args)
    {
        CommandLine commandLine = new CommandLine(new MainCommand());
        allowUnmatchedArguments(commandLine);
        commandLine.execute(args);
    }

    /* The dotted --config.flag=value arguments don't match any declared
     * option, so allow them to be collected as unmatched arguments. */
    private static void allowUnmatchedArguments(CommandLine commandLine)
    {
        commandLine.setUnmatchedArgumentsAllowed(true);
        for (CommandLine sub : commandLine.getSubcommands().values())
            allowUnmatchedArguments(sub);
    }
}
