package com.cowlark.fluxengine.core;

/**
 * The base exception for FluxEngine errors.
 */
public class FluxEngineException extends RuntimeException
{
    public FluxEngineException(String message)
    {
        super(message);
    }

    public FluxEngineException(String message, Throwable cause)
    {
        super(message, cause);
    }
}
