package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigException;
import com.cowlark.fluxengine.config.ConfigProtoOrBuilder;
import com.fazecast.jSerialComm.SerialPort;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.Iterables;
import lombok.Data;
import lombok.experimental.Accessors;
import org.usb4java.Context;
import org.usb4java.Device;
import org.usb4java.DeviceDescriptor;
import org.usb4java.DeviceHandle;
import org.usb4java.DeviceList;
import org.usb4java.LibUsb;
import java.nio.ByteBuffer;
import java.util.Set;

public class UsbFinder
{
    private static final int GREASEWEAZLE_ID = 0x12094d69;
    private static final int FLUXENGINE_ID = 0x12096e00;
    private static final int APPLESAUCE_ID = 0x16c00483;
    private static final Set<Integer> VALID_DEVICES =
            Set.of(GREASEWEAZLE_ID, FLUXENGINE_ID, APPLESAUCE_ID);

    private static String getSerialNumber(Device device, DeviceDescriptor descriptor)
    {
        String s = HackyUsbSerialNumberResolver.resolve(device, descriptor);
        if (s == null)
            return "n/a";
        return s;
    }

    public static synchronized ImmutableList<CandidateDevice> findUsbDevices()
    {
        ImmutableList.Builder<CandidateDevice> candidates = ImmutableList.builder();
        Context ctx = UsbContext.get();
        DeviceList list = new DeviceList();
        int rc = LibUsb.getDeviceList(ctx, list);
        if (rc < 0)
        {
            System.err.println("USB error: " + LibUsb.errorName(rc));
            return candidates.build();
        }
        try
        {
            for (Device device : list)
            {
                DeviceDescriptor descriptor = new DeviceDescriptor();
                int r = LibUsb.getDeviceDescriptor(device, descriptor);
                if (r != LibUsb.SUCCESS)
                    continue;

                int id = ((descriptor.idVendor() & 0xffff) << 16)
                        | (descriptor.idProduct() & 0xffff);
                if (!VALID_DEVICES.contains(id))
                    continue;

                CandidateDevice candidate = new CandidateDevice();
                candidate.id = id;
                candidate.serial = getSerialNumber(device, descriptor);
                candidate.busNumber = LibUsb.getBusNumber(device);
                candidate.deviceAddress = LibUsb.getDeviceAddress(device);
                candidate.portPath = getPortPath(device);

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
        } finally
        {
            LibUsb.freeDeviceList(list, true);
        }
        return candidates.build();
    }

    /* Selects a device to use, based on the configuration, ported from
     * lib/usb/usb.cc. */
    public static CandidateDevice selectDevice(ConfigProtoOrBuilder config)
    {
        ImmutableList<CandidateDevice> candidates = findUsbDevices();
        if (candidates.isEmpty())
            throw new ConfigException("no devices found (is one plugged in? Do you have the "
                    + "appropriate permissions?");

        String wantedSerial = config.getUsb().getSerial();
        if (!Strings.isNullOrEmpty(wantedSerial))
        {
            CandidateDevice found = null;
            for (CandidateDevice candidate : candidates)
            {
                if (candidate.serial.equals(wantedSerial))
                {
                    found = candidate;
                    break;
                }
            }
            if (found == null)
                throw new ConfigException("serial number not found");
            return found;
        }

        if (candidates.size() == 1)
            return Iterables.getOnlyElement(candidates);

        throw new ConfigException(
                "more than one device detected; you'll need to explicitly specify the serial "
                        + "number of the device you want");
    }

    /* Opens the USB device indicated by the candidate's location. The caller
     * receives an open DeviceHandle; the underlying Device is not retained. */
    static void openDevice(CandidateDevice candidate, DeviceHandle handle)
    {
        Context ctx = UsbContext.get();
        DeviceList list = new DeviceList();
        int rc = LibUsb.getDeviceList(ctx, list);
        if (rc < 0)
            throw new com.cowlark.fluxengine.core.FluxEngineException(
                    "USB error: " + LibUsb.errorName(rc));
        try
        {
            for (Device device : list)
            {
                if (LibUsb.getBusNumber(device) != candidate.busNumber)
                    continue;
                if (LibUsb.getDeviceAddress(device) != candidate.deviceAddress)
                    continue;
                /* If we have a port path, verify it as well for robustness
                 * against address reuse. */
                if (candidate.portPath != null && !candidate.portPath.isEmpty())
                {
                    ImmutableList<Integer> actual = getPortPath(device);
                    if (!actual.equals(candidate.portPath))
                        continue;
                }
                int openRc = LibUsb.open(device, handle);
                if (openRc != LibUsb.SUCCESS)
                    throw new com.cowlark.fluxengine.core.FluxEngineException(
                            "FluxEngine: USB open failed: " + LibUsb.errorName(openRc) + " "
                                    + LibUsb.strError(openRc));
                return;
            }
            throw new com.cowlark.fluxengine.core.FluxEngineException(
                    String.format(
                            "USB device at bus %d address %d not found (was it unplugged?)",
                            candidate.busNumber,
                            candidate.deviceAddress));
        } finally
        {
            LibUsb.freeDeviceList(list, true);
        }
    }

    private static ImmutableList<Integer> getPortPath(Device device)
    {
        ByteBuffer buffer = ByteBuffer.allocateDirect(8);
        int len = LibUsb.getPortNumbers(device, buffer);
        if (len <= 0)
            return ImmutableList.of();
        ImmutableList.Builder<Integer> builder = ImmutableList.builder();
        for (int i = 0; i < len; i++)
            builder.add((int) buffer.get(i) & 0xff);
        return builder.build();
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
                if (serial == null || serial.isEmpty() || portSerial == null
                        || serial.equals(portSerial))
                {
                    return port.getSystemPortName();
                }
            }
        }
        return null;
    }

    /* Retained for API compatibility; previously freed retained Device refs.
     * Now a no-op as CandidateDevice no longer holds a Device reference. */
    public static void freeDevices(Iterable<CandidateDevice> devices)
    {
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
        public int id;
        public String serial;
        public String serialPort;
        public int busNumber;
        public int deviceAddress;
        public ImmutableList<Integer> portPath = ImmutableList.of();

        /* Retained for API compatibility; previously released the retained
         * Device ref. Now a no-op as CandidateDevice no longer holds a
         * Device reference. */
        public void release()
        {
        }
    }
}
