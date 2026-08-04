package com.cowlark.fluxengine;

import com.cowlark.fluxengine.cli.MainCommand;
import com.cowlark.fluxengine.cli.TestCommand;
import com.cowlark.fluxengine.cli.TestDevicesCommand;
import dagger.Component;
import javax.inject.Singleton;

@Singleton
@Component
interface FluxEngineComponent
{
    static FluxEngineComponent create()
    {
        return DaggerFluxEngineComponent.create();
    }

    MainCommand mainCommand();

    TestCommand testCommand();

    TestDevicesCommand testDevicesCommand();
}
