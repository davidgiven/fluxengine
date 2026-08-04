package com.cowlark.fluxengine;

import com.cowlark.fluxengine.wiring.CliParameters;
import dagger.BindsInstance;
import dagger.Component;
import javax.inject.Singleton;

/**
 * Component that exposes the concrete Fluxengine type.
 * Dagger will generate DaggerFluxengineComponent when annotation processing runs.
 */
@Singleton
@Component(modules = FluxengineModule.class)
interface FluxengineComponent
{
    Fluxengine fluxengine();

    @Component.Builder
    interface Builder
    {
        @BindsInstance
        Builder cliParameters(@CliParameters String[] args);

        FluxengineComponent build();
    }

    // Convenience factory that delegates to the generated implementation
    static FluxengineComponent create(String[] args)
    {
        return DaggerFluxengineComponent.builder().cliParameters(args).build();
    }
}
