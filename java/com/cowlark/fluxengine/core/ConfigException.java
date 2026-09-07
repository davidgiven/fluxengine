package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.FluxEngineException;

/**
 * An error relating to loading or processing the configuration.
 */
public class ConfigException extends FluxEngineException
{
    public ConfigException(String message)
    {
        super(message);
    }

    public ConfigException(String message, Throwable cause)
    {
        super(message, cause);
    }
}
