package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.wiring.Scoped;
import dagger.Subcomponent;
import picocli.CommandLine;
import picocli.CommandLine.IFactory;

/**
 * Dagger subcomponent for CLI-related accessors.
 */
@Scoped
@Subcomponent
public interface CliComponent extends IFactory
{
    MainCommand mainCommand();

    TestCommand testCommand();

    TestDevicesCommand testDevicesCommand();

    TestBandwidthCommand testBandwidthCommand();

    @Subcomponent.Factory
    interface Factory
    {
        CliComponent create();
    }

    @Override
    @SuppressWarnings("unchecked")
    default <K> K create(Class<K> cls) throws Exception
    {
        if (cls == TestCommand.class)
            return (K) testCommand();
        if (cls == TestDevicesCommand.class)
            return (K) testDevicesCommand();
        if (cls == TestBandwidthCommand.class)
            return (K) testBandwidthCommand();
        return CommandLine.defaultFactory().create(cls);
    }
}
