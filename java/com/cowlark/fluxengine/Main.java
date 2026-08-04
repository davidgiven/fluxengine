package com.cowlark.fluxengine;

import com.cowlark.fluxengine.cli.TestCommand;
import com.cowlark.fluxengine.cli.TestDevicesCommand;
import picocli.CommandLine;

public class Main
{
    public static void main(String[] args)
    {
        FluxEngineComponent component = FluxEngineComponent.create();
        CommandLine commandLine =
            new CommandLine(component.mainCommand(), new CommandFactory(component));
        commandLine.execute(args);
    }

    private static final class CommandFactory implements CommandLine.IFactory
    {
        private final FluxEngineComponent component;

        CommandFactory(FluxEngineComponent component)
        {
            this.component = component;
        }

        @Override
        @SuppressWarnings("unchecked")
        public <K> K create(Class<K> cls) throws Exception
        {
            if (cls == TestCommand.class)
                return (K) component.testCommand();
            if (cls == TestDevicesCommand.class)
                return (K) component.testDevicesCommand();
            return CommandLine.defaultFactory().create(cls);
        }
    }
}
