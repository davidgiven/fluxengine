package com.cowlark.fluxengine.config;

import com.fazecast.jSerialComm.SerialPort;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import lombok.Data;
import lombok.experimental.Accessors;
import org.usb4java.javax.Services;
import javax.usb.UsbDeviceDescriptor;
import javax.usb.UsbException;
import javax.usb.UsbHub;
import javax.usb.UsbServices;
import java.util.Set;

public class UsbFinder
{
    private static final int GREASEWEAZLE_ID = 0x12094d69;
    private static final int FLUXENGINE_ID = 0x12096e00;
    private static final int APPLESAUCE_ID = 0x16c00483;
    private static final Set<Integer> VALID_DEVICES =
            Set.of(GREASEWEAZLE_ID, FLUXENGINE_ID, APPLESAUCE_ID);

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

    public static synchronized ImmutableList<CandidateDevice> findUsbDevices()
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

    /* Selects a device to use, based on the configuration, ported from
     * lib/usb/usb.cc. */
    public static CandidateDevice selectDevice(ConfigProtoOrBuilder config)
    {
        ImmutableList<CandidateDevice> candidates = findUsbDevices();
        if (candidates.isEmpty())
            throw new ConfigException("no devices found (is one plugged in? Do you have the " +
                    "appropriate permissions?");

        String wantedSerial = config.getUsb().getSerial();
        if (!Strings.isNullOrEmpty(wantedSerial))
        {
            for (CandidateDevice candidate : candidates)
            {
                if (candidate.serial.equals(wantedSerial))
                    return candidate;
            }
            throw new ConfigException("serial number not found");
        }

        if (candidates.size() == 1)
            return Iterables.getOnlyElement(candidates);

        throw new ConfigException(
                "more than one device detected; you'll need to explicitly specify the serial " +
                        "number of the device you want");
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

    @Accessors(chain = true)
    @Data
    public static final class CandidateDevice
    {
        public DeviceType type;
        public javax.usb.UsbDevice device;
        public int id;
        public String serial;
        public String serialPort;
    }
}
