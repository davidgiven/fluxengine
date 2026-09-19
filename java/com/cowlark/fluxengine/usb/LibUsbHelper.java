package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.FluxEngineException;
import org.usb4java.LibUsb;

/* Central helper for checking LibUsb return codes, which are 0 (SUCCESS) on
 * success and a negative libusb error code on failure. Most call sites
 * previously duplicated the if (rc != SUCCESS) throw pattern; this class
 * consolidates it. */
final class LibUsbHelper
{
    private LibUsbHelper()
    {
    }

    /* Throws FluxEngineException with a formatted message if rc is not
     * SUCCESS. The message is prefixed with context and suffixed with the
     * libusb error name and string. */
    static void check(int rc, String context)
    {
        if (rc != LibUsb.SUCCESS)
        {
            throw new FluxEngineException(context + ": " + LibUsb.errorName(rc) + " "
                    + LibUsb.strError(rc));
        }
    }

    /* Checks a resetDevice return code. libusb notes that reset may cause
     * re-enumeration, in which case the handle is no longer valid and the
     * caller should rediscover the device. That case is signalled with
     * ERROR_NOT_FOUND or ERROR_NO_DEVICE and is mapped to RetryableUsbException
     * so UsbFactory can retry. All other failures map to FluxEngineException. */
    static void checkReset(int rc)
    {
        if (rc == LibUsb.ERROR_NOT_FOUND || rc == LibUsb.ERROR_NO_DEVICE)
        {
            throw new RetryableUsbException(
                    "FluxEngine: USB reset caused re-enumeration, retrying: "
                            + LibUsb.errorName(rc));
        }
        check(rc, "FluxEngine: reset failed");
    }
}
