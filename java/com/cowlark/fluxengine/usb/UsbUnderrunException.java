package com.cowlark.fluxengine.usb;

public class UsbUnderrunException extends RetryableUsbException
{
    public UsbUnderrunException()
    {
        super("USB underrun (not enough bandwidth)");
    }
}
