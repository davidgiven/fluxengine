package com.cowlark.fluxengine;

import com.cowlark.fluxengine.wiring.CliParameters;
import dagger.BindsInstance;
import dagger.Component;
import javax.inject.Singleton;

/**
 * Component that exposes the concrete FluxEngine type.
 * Dagger will generate DaggerFluxEngineComponent when annotation processing runs.
 */
@Singleton
@Component(modules = FluxEngineModule.class)
interface FluxEngineComponent
{
    FluxEngine fluxengine();

    @Component.Builder
    interface Builder
    {
        @BindsInstance
        Builder cliParameters(@CliParameters String[] args);

        FluxEngineComponent build();
    }

    // Convenience factory that delegates to the generated implementation
    static FluxEngineComponent create(String[] args)
    {
        return DaggerFluxEngineComponent.builder().cliParameters(args).build();
    }
}
