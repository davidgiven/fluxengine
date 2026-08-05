package com.cowlark.fluxengine.usb;

import static com.google.common.base.Strings.nullToEmpty;

import com.cowlark.fluxengine.config.Config;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.fazecast.jSerialComm.SerialPort;
import com.google.common.collect.ImmutableList;
import org.usb4java.javax.Services;
import javax.usb.UsbDeviceDescriptor;
import javax.usb.UsbException;
import javax.usb.UsbHub;
import javax.usb.UsbServices;
import java.util.Set;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public final class UsbFactory
{
    public enum DeviceType
    {
        FLUXENGINE("FluxEngine"), GREASEWEAZLE("Greaseweazle"), APPLESAUCE("Applesauce");

        private final String deviceName;

        DeviceType(String deviceName)
        {
            this.deviceName = deviceName;
        }

        public String getDeviceName()
        {
            return deviceName;
        }
    }

    public static final class CandidateDevice
    {
        public DeviceType type;
        public javax.usb.UsbDevice device;
        public int id;
        public String serial;
        public String serialPort;
    }

    private static final int GREASEWEAZLE_ID = 0x12094d69;
    private static final int FLUXENGINE_ID = 0x12096e00;
    private static final int APPLESAUCE_ID = 0x16c00483;

    private static final Set<Integer> VALID_DEVICES =
            Set.of(GREASEWEAZLE_ID, FLUXENGINE_ID, APPLESAUCE_ID);

    private UsbFactory()
    {
    }

    private static String getSerialNumber(javax.usb.UsbDevice device)
    {
        try
        {
            return device.getSerialNumberString();
        } catch (UsbException | java.io.UnsupportedEncodingException e)
        {
            return "n/a";
        }
    }

    public static ImmutableList<CandidateDevice> findUsbDevices()
    {
        ImmutableList.Builder<CandidateDevice> candidates = ImmutableList.builder();
        try
        {
            UsbServices services = new Services();
            UsbHub rootHub = services.getRootUsbHub();
            walkHub(rootHub, candidates);
        } catch (UsbException e)
        {
            System.err.println("USB error: " + e.getMessage());
        }
        return candidates.build();
    }

    public static UsbDevice connect(CandidateDevice device)
    {
        return null;
    }

    public static UsbDevice connect(Config config)
    {
        return connect(selectDevice(config));
    }

    /* Selects a device to use, based on the configuration, ported from
     * lib/usb/usb.cc. */
    public static CandidateDevice selectDevice(Config config)
    {
        ImmutableList<CandidateDevice> candidates = findUsbDevices();
        if (candidates.isEmpty())
            throw new FluxEngineException(
                    "no devices found (is one plugged in? Do you have the " +
                            "appropriate permissions?");

        String wantedSerial = config.get("usb.serial");
        if (wantedSerial != null)
        {
            for (CandidateDevice candidate : candidates)
            {
                if (candidate.serial.equals(wantedSerial))
                    return candidate;
            }
            throw new FluxEngineException(
                    "serial number not found (try without one to list or " +
                            "autodetect devices)");
        }

        if (candidates.size() == 1)
            return candidates.get(0);

        System.err.println(
                "More than one device detected; use --usb.serial=<serial> to " +
                        "select one:");
        for (CandidateDevice candidate : candidates)
        {
            System.err.print("    ");
            switch (candidate.type)
            {
                case FLUXENGINE:
                    System.err.printf("FluxEngine: %s\n", candidate.serial);
                    break;

                case GREASEWEAZLE:
                    System.err.printf("Greaseweazle: %s on %s\n",
                            candidate.serial,
                            nullToEmpty(candidate.serialPort));
                    break;

                case APPLESAUCE:
                    System.err.printf("Applesauce: %s on %s\n",
                            candidate.serial,
                            nullToEmpty(candidate.serialPort));
                    break;
            }
        }
        System.exit(1);
        return null; /* unreachable */
    }

    private static void walkHub(UsbHub hub, ImmutableList.Builder<CandidateDevice> candidates)
    {
        for (Object o : hub.getAttachedUsbDevices())
        {
            javax.usb.UsbDevice usbDevice = (javax.usb.UsbDevice) o;
            if (usbDevice.isUsbHub())
                walkHub((UsbHub) usbDevice, candidates);

            UsbDeviceDescriptor descriptor = usbDevice.getUsbDeviceDescriptor();
            int id = ((descriptor.idVendor() & 0xffff) << 16) | (descriptor.idProduct() & 0xffff);
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
                candidate.serialPort = findSerialPort(id, candidate.serial);

            candidates.add(candidate);
        }
    }

    private static String findSerialPort(int id, String serial)
    {
        int vendorId = id >>> 16;
        int productId = id & 0xffff;
        for (SerialPort port : SerialPort.getCommPorts())
        {
            if (port.getVendorID() == vendorId && port.getProductID() == productId)
            {
                String portSerial = port.getSerialNumber();
                if (serial == null || serial.isEmpty() || portSerial == null ||
                        serial.equals(portSerial))
                {
                    return port.getSystemPortName();
                }
            }
        }
        return null;
    }
}
