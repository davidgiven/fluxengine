package com.cowlark.fluxengine;

import com.cowlark.fluxengine.cli.MainCommand;
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
}
