package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.Bytes;

/**
 * Base class for USB floppy drive devices, ported from lib/usb/usb.h.
 */
public abstract class UsbDevice implements AutoCloseable
{
    /* I/O operations --- these all apply the settings in the DriveSetting sobject before doing
    anything. */

    /* Just sets up the drive, and does nothing else. */
    public abstract void seek(DriveSettings state);

    /* Reads readTimeNs-worth of samples. */
    public abstract Bytes read(DriveSettings state, double readTimeNs);

    /* Writes samples (must be less than or equal to one revolution). */
    public abstract void write(DriveSettings state, Bytes bytes);

    /* Magnetically erases the current track. */
    public abstract void erase(DriveSettings state);

    /* Measures the rotational period of the drive. */
    public abstract double getRotationalPeriod(DriveSettings settings);

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
