package com.cowlark.fluxengine.core;

/**
 * Thrown by Common.testForEmergencyStop when the emergency stop flag is set,
 * causing the current operation to unwind, ported from lib/core/utils.h.
 */
public class EmergencyStopException extends RuntimeException
{
    public EmergencyStopException()
    {
        super("aborted by emergency stop");
    }
}
