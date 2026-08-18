package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.Bytes;

/**
 * Base class for USB floppy drive devices, ported from lib/usb/usb.h.
 */
public abstract class UsbDevice implements AutoCloseable
{
    /* Just sets up the drive, and does nothing else. */
    public abstract void seek(int cylinder);

    /* Reads readTimeNs-worth of samples. */
    public abstract Bytes read(int cylinder, int head, double readTimeNs);

    /* Writes samples (must be less than or equal to one revolution). */
    public abstract void write(int cylinder, int head, Bytes bytes);

    /* Magnetically erases the current track. */
    public abstract void erase(int cylinder, int head);

    /* Measures the rotational period of the drive. */
    public abstract double getRotationalPeriod();

    /* Utility operations. */

    /* Tests data transfers to the device buffer. */
    public abstract void testBulkWrite();

    /* Tests data transfers from the device buffer. */
    public abstract void testBulkRead();

    /* Measures the voltages used by the device. */
    public abstract VoltageMeasurements measureVoltages();

    /* Closes the device, releasing any underlying resources. */
    @Override
    public abstract void close();

    protected String usbError(int error)
    {
        return String.format("USB error %d", error);
    }
}
