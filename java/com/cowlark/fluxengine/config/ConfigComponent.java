package com.cowlark.fluxengine.config;

import dagger.Subcomponent;
import com.cowlark.fluxengine.wiring.Scoped;

/**
 * Dagger subcomponent for configuration-related accessors.
 */
@Scoped
@Subcomponent
public interface ConfigComponent
{
    Config config();

    ConfigFactory configFactory();

    @Subcomponent.Factory
    interface Factory
    {
        ConfigComponent create();
    }
}
