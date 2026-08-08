package com.cowlark.fluxengine.core;

/**
 * Thrown to abort a running operation, ported from lib/core/utils.h.
 */
public class EmergencyStopException extends RuntimeException
{
    public EmergencyStopException()
    {
        super();
    }
}
