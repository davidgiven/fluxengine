package com.cowlark.fluxengine.usb;

import org.slf4j.LoggerFactory;
import org.usb4java.Context;
import org.usb4java.LibUsb;

/* Singleton holder for the explicit libusb Context, ported from libusb_init
 * usage in lib/usb/usb.cc. The Context is initialised once via
 * LibUsb.init and lives until JVM shutdown; this keeps any retained
 * org.usb4java.Device objects (which require a live Context) valid between
 * UsbFinder.findUsbDevices and FluxEngineUsbDevice.open. */
final class UsbContext
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(UsbContext.class);

    private static final Context CONTEXT;

    static
    {
        Context ctx = new Context();
        int rc = LibUsb.init(ctx);
        LibUsbHelper.check(rc, "LibUsb.init failed");
        CONTEXT = ctx;
        Runtime.getRuntime().addShutdownHook(new Thread(() ->
        {
            logger.atDebug().log("exiting libusb Context");
            LibUsb.exit(CONTEXT);
        }));
    }

    private UsbContext()
    {
    }

    static Context get()
    {
        return CONTEXT;
    }
}
