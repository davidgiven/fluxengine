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
import org.usb4java.DeviceList;
import org.usb4java.LibUsb;
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

                /* Retain the Device beyond freeDeviceList; the caller (or
                 * FluxEngineUsbDevice) is responsible for LibUsb.unrefDevice. */
                LibUsb.refDevice(device);

                CandidateDevice candidate = new CandidateDevice();
                candidate.device = device;
                candidate.id = id;
                candidate.serial = getSerialNumber(device, descriptor);

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
            /* unrefDevices=true decrements the ref added by getDeviceList;
             * candidates we ref'd above stay alive with one ref. */
            LibUsb.freeDeviceList(list, true);
        }
        return candidates.build();
    }

    /* Selects a device to use, based on the configuration, ported from
     * lib/usb/usb.cc. The returned CandidateDevice retains a Device ref;
     * unused candidates are unref'd. */
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
            {
                /* Unref all retained devices before throwing. */
                freeDevices(candidates);
                throw new ConfigException("serial number not found");
            }
            /* Unref the non-selected candidates. */
            for (CandidateDevice c : candidates)
                if (c != found && c.device != null)
                    LibUsb.unrefDevice(c.device);
            return found;
        }

        if (candidates.size() == 1)
            return Iterables.getOnlyElement(candidates);

        /* More than one candidate and no serial specified: unref all and fail.
         * The caller does not receive a candidate, so no Device is leaked. */
        freeDevices(candidates);
        throw new ConfigException(
                "more than one device detected; you'll need to explicitly specify the serial "
                        + "number of the device you want");
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

    /* Frees the retained Device refs for a collection of candidates. Call this
     * when you obtained a list via findUsbDevices and are done with it (e.g.
     * DevicesCommand). Candidates whose device is null (synthetic entries such
     * as DEVICE_FLUXFILE) are ignored. */
    public static void freeDevices(Iterable<CandidateDevice> devices)
    {
        for (CandidateDevice c : devices)
            if (c.device != null)
            {
                LibUsb.unrefDevice(c.device);
                c.device = null;
            }
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
        public Device device;
        public int id;
        public String serial;
        public String serialPort;

        /* Releases the retained Device ref if any. Safe to call multiple times. */
        public void release()
        {
            if (device != null)
            {
                LibUsb.unrefDevice(device);
                device = null;
            }
        }
    }
}
