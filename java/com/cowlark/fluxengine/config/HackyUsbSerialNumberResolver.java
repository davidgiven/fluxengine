package com.cowlark.fluxengine.config;

import com.sun.jna.Native;
import com.sun.jna.Pointer;
import com.sun.jna.Structure;
import com.sun.jna.platform.win32.Guid.GUID;
import com.sun.jna.win32.StdCallLibrary;
import com.sun.jna.win32.W32APIOptions;

import javax.usb.UsbDevice;
import javax.usb.UsbDeviceDescriptor;
import javax.usb.UsbException;
import java.io.UnsupportedEncodingException;
import java.util.Arrays;
import java.util.List;
import java.util.Locale;
import java.util.regex.Matcher;
import java.util.regex.Pattern;

/// Resolves a USB device's serial number across Windows, macOS, and Linux.
///
/// Strategy: try javax.usb's UsbDevice.getSerialNumberString() first — this
/// only requires a control transfer on endpoint 0, which macOS and Linux's
/// libusb backends allow even while a class driver (CDC/ACM, FTDI, etc.)
/// still owns the device's interfaces. Windows is the exception: libusb
/// there can't open a device at all unless it's bound to WinUSB/libusbK/
/// libusb-win32, so the direct call throws for devices left on their normal
/// driver (e.g. a standard serial port device). In that case, fall back to
/// a native SetupAPI lookup keyed on the VID/PID you already have from
/// javax.usb — no driver changes required for this path.
///
/// Requires net.java.dev.jna:jna and jna-platform (only exercised on
/// Windows; harmless to have as a dependency on other platforms).
public final class HackyUsbSerialNumberResolver
{

    private HackyUsbSerialNumberResolver()
    {
    }

    /**
     * @return the device's serial number, or null if it genuinely has none
     * (iSerialNumber == 0 in its device descriptor).
     */
    public static String resolve(UsbDevice device) throws UsbException, UnsupportedEncodingException
    {
        try
        {
            String serial = device.getSerialNumberString();
            if (serial != null)
            {
                return serial;
            }
        } catch (UsbException e)
        {
            if (!isWindows())
            {
                throw e;
            }
        }

        if (isWindows())
        {
            UsbDeviceDescriptor descriptor = device.getUsbDeviceDescriptor();
            int vendorId = descriptor.idVendor() & 0xFFFF;
            int productId = descriptor.idProduct() & 0xFFFF;
            return Windows.getSerialNumberForUsbId(vendorId, productId);
        }

        return null;
    }

    private static boolean isWindows()
    {
        return System.getProperty("os.name", "").toLowerCase(Locale.ROOT).contains("win");
    }

    /* Windows fallback using internal APIs. */
    private static final class Windows
    {
        private static final int DIGCF_PRESENT = 0x00000002;
        private static final int DIGCF_ALLCLASSES = 0x00000004;
        private static final int CR_SUCCESS = 0;

        public static class SP_DEVINFO_DATA extends Structure
        {
            public int cbSize;
            public GUID.ByValue classGuid;
            public int devInst;
            public Pointer reserved;

            public SP_DEVINFO_DATA()
            {
                cbSize = size();
            }

            @Override
            protected List<String> getFieldOrder()
            {
                return Arrays.asList("cbSize", "classGuid", "devInst", "reserved");
            }
        }

        private interface SetupApi extends StdCallLibrary
        {
            SetupApi INSTANCE =
                    Native.load("setupapi", SetupApi.class, W32APIOptions.UNICODE_OPTIONS);

            Pointer SetupDiGetClassDevsW(
                    GUID classGuid,
                    String enumerator,
                    Pointer hwndParent,
                    int flags);

            boolean SetupDiEnumDeviceInfo(
                    Pointer deviceInfoSet,
                    int memberIndex,
                    SP_DEVINFO_DATA deviceInfoData);

            boolean SetupDiGetDeviceInstanceIdW(
                    Pointer deviceInfoSet,
                    SP_DEVINFO_DATA deviceInfoData,
                    char[] deviceInstanceId,
                    int deviceInstanceIdSize,
                    com.sun.jna.platform.win32.WinDef.DWORDByReference requiredSize);

            boolean SetupDiDestroyDeviceInfoList(Pointer deviceInfoSet);
        }

        static String getSerialNumberForUsbId(int vendorId, int productId)
        {
            Pattern pattern = Pattern.compile(
                    String.format("^USB\\\\VID_%04X&PID_%04X\\\\([^\\\\]+)$", vendorId, productId),
                    Pattern.CASE_INSENSITIVE);

            // classGuid = null + enumerator "USB" + DIGCF_ALLCLASSES enumerates
            // every USB device node present, regardless of device class.
            Pointer deviceInfoSet = SetupApi.INSTANCE.SetupDiGetClassDevsW(
                    null,
                    "USB",
                    Pointer.NULL,
                    DIGCF_PRESENT | DIGCF_ALLCLASSES);
            if (deviceInfoSet == Pointer.NULL)
            {
                throw new RuntimeException("SetupDiGetClassDevs failed");
            }

            try
            {
                int index = 0;
                SP_DEVINFO_DATA data = new SP_DEVINFO_DATA();

                while (SetupApi.INSTANCE.SetupDiEnumDeviceInfo(deviceInfoSet, index++, data))
                {
                    String instanceId = getInstanceId(deviceInfoSet, data);
                    if (instanceId != null)
                    {
                        Matcher m = pattern.matcher(instanceId);
                        if (m.matches())
                        {
                            return m.group(1);
                        }
                    }
                    data = new SP_DEVINFO_DATA();
                }
            } finally
            {
                SetupApi.INSTANCE.SetupDiDestroyDeviceInfoList(deviceInfoSet);
            }

            return null;
        }

        private static String getInstanceId(Pointer deviceInfoSet, SP_DEVINFO_DATA data)
        {
            char[] buffer = new char[512];
            com.sun.jna.platform.win32.WinDef.DWORDByReference required =
                    new com.sun.jna.platform.win32.WinDef.DWORDByReference();
            boolean ok = SetupApi.INSTANCE.SetupDiGetDeviceInstanceIdW(
                    deviceInfoSet,
                    data,
                    buffer,
                    buffer.length,
                    required);
            return ok ? Native.toString(buffer) : null;
        }
    }

    /**
     * True if the value looks like a genuine device-reported serial rather
     * than a Windows-generated placeholder (e.g. "6&2c3d1a4&0&0002").
     */
    public static boolean looksLikeGenuineSerial(String serial)
    {
        return serial != null && !serial.contains("&");
    }
}
