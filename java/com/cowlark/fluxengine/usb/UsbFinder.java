package com.cowlark.fluxengine.usb;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import javax.inject.Inject;
import javax.usb.UsbDevice;
import javax.usb.UsbDeviceDescriptor;
import javax.usb.UsbException;
import javax.usb.UsbHub;
import javax.usb.UsbServices;
import org.usb4java.javax.Services;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public final class UsbFinder
{
    public enum DeviceType
    {
        FLUXENGINE,
        GREASEWEAZLE,
        APPLESAUCE,
    }

    public static final class CandidateDevice
    {
        public DeviceType type;
        public UsbDevice device;
        public int id;
        public String serial;
        public String serialPort;
    }

    private static final int GREASEWEAZLE_ID = 0x12094d69;
    private static final int FLUXENGINE_ID = 0x12096e00;
    private static final int APPLESAUCE_ID = 0x16c00483;

    private static final Set<Integer> VALID_DEVICES =
        Set.of(GREASEWEAZLE_ID, FLUXENGINE_ID, APPLESAUCE_ID);

    @Inject
    public UsbFinder()
    {
    }

    public String getDeviceName(DeviceType type)
    {
        switch (type)
        {
            case GREASEWEAZLE:
                return "Greaseweazle";

            case FLUXENGINE:
                return "FluxEngine";

            case APPLESAUCE:
                return "Applesauce";

            default:
                return "unknown";
        }
    }

    private static String getSerialNumber(UsbDevice device)
    {
        try
        {
            return device.getSerialNumberString();
        }
        catch (UsbException | java.io.UnsupportedEncodingException e)
        {
            return "n/a";
        }
    }

    public List<CandidateDevice> findUsbDevices()
    {
        List<CandidateDevice> candidates = new ArrayList<>();
        try
        {
            UsbServices services = new Services();
            UsbHub rootHub = services.getRootUsbHub();
            walkHub(rootHub, candidates);
        }
        catch (UsbException e)
        {
            System.err.println("USB error: " + e.getMessage());
        }
        return candidates;
    }

    private static void walkHub(UsbHub hub, List<CandidateDevice> candidates)
    {
        for (Object o : hub.getAttachedUsbDevices())
        {
            UsbDevice usbDevice = (UsbDevice) o;
            if (usbDevice.isUsbHub())
                walkHub((UsbHub) usbDevice, candidates);

            UsbDeviceDescriptor descriptor = usbDevice.getUsbDeviceDescriptor();
            int id = ((descriptor.idVendor() & 0xffff) << 16) |
                     (descriptor.idProduct() & 0xffff);
            if (!VALID_DEVICES.contains(id))
                continue;

            CandidateDevice candidate = new CandidateDevice();
            candidate.device = usbDevice;
            candidate.id = id;
            candidate.serial = getSerialNumber(usbDevice);

            if (id == GREASEWEAZLE_ID)
                candidate.type = DeviceType.GREASEWEAZLE;
            else if (id == APPLESAUCE_ID)
                candidate.type = DeviceType.APPLESAUCE;
            else
                candidate.type = DeviceType.FLUXENGINE;

            if (id == GREASEWEAZLE_ID || id == APPLESAUCE_ID)
            {
                // TODO: map the USB device to an OS serial port (CDC-ACM).
                candidate.serialPort = null;
            }

            candidates.add(candidate);
        }
    }
}
