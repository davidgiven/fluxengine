package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.FluxEngineException;

public class RetryableUsbException extends FluxEngineException
{
    public RetryableUsbException(String message)
    {
        super(message);
    }
}
