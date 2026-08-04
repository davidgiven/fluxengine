package com.cowlark.fluxengine;

import dagger.Module;
import dagger.Provides;
import javax.inject.Singleton;

/**
 * Application module that provides application-scoped dependencies.
 */
@Module
class FluxEngineModule
{
    @Provides
    @Singleton
    Greeter provideGreeter()
    {
        return new Greeter();
    }
}
