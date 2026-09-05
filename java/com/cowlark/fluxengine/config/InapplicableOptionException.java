package com.cowlark.fluxengine.config;

/**
 * Thrown when an option cannot be applied to the current configuration,
 * ported from lib/config/config.h.
 */
public class InapplicableOptionException extends ConfigException
{
    public InapplicableOptionException(String message, Object... args)
    {
        super(String.format(message, args));
    }
}
