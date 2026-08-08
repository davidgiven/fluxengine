package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.Bytes;

/**
 * Base class for USB floppy drive devices, ported from lib/usb/usb.h.
 */
public abstract class UsbDevice
{
    public void recalibrate()
    {
        seek(0);
    }

    public abstract void seek(int track);

    public abstract double getRotationalPeriod(int hardSectorCount);

    public abstract void testBulkWrite();

    public abstract void testBulkRead();

    public abstract Bytes read(int side,
                               boolean synced,
                               double readTimeNs,
                               double hardSectorThresholdNs);

    public abstract void write(int side, Bytes bytes, double hardSectorThresholdNs);

    public abstract void erase(int side, double hardSectorThresholdNs);

    public abstract void setDrive(int drive, boolean highDensity, int indexMode);

    public abstract VoltageMeasurements measureVoltages();

    protected String usbError(int error)
    {
        return String.format("USB error %d", error);
    }
}
