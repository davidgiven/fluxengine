package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.FluxEngineException;
import org.slf4j.LoggerFactory;
import org.usb4java.Context;
import org.usb4java.LibUsb;

/* Singleton holder for the explicit libusb Context, ported from libusb_init
 * usage in lib/usb/usb.cc. The Context is initialised once via
 * LibUsb.init and lives until JVM shutdown. */
final class UsbContext
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(UsbContext.class);

    private static final Context CONTEXT;

    static
    {
        Context ctx = new Context();
        logger.atDebug().log("initialising libusb Context");
        int rc = LibUsb.init(ctx);
        if (rc != LibUsb.SUCCESS)
            throw new FluxEngineException("Failed to initialize libusb");

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
