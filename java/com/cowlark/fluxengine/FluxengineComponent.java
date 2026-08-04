package com.cowlark.fluxengine;

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

    // Convenience factory that delegates to the generated implementation
    static FluxengineComponent create()
    {
        return DaggerFluxengineComponent.create();
    }
}
