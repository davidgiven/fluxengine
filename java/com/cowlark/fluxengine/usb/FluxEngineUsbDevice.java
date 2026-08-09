package com.cowlark.fluxengine.usb;

import static com.cowlark.fluxengine.external.FluxEngine.FLUXENGINE_CMD_IN_EP;
import static com.cowlark.fluxengine.external.FluxEngine.FLUXENGINE_CMD_OUT_EP;
import static com.cowlark.fluxengine.external.FluxEngine.FLUXENGINE_DATA_IN_EP;
import static com.cowlark.fluxengine.external.FluxEngine.FLUXENGINE_DATA_OUT_EP;
import static com.cowlark.fluxengine.external.FluxEngine.FLUXENGINE_PROTOCOL_VERSION;
import static com.cowlark.fluxengine.external.FluxEngine.FRAME_SIZE;
import static com.cowlark.fluxengine.external.FluxEngine.F_ERROR_BAD_COMMAND;
import static com.cowlark.fluxengine.external.FluxEngine.F_ERROR_UNDERRUN;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_BULK_READ_TEST_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_BULK_READ_TEST_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_BULK_WRITE_TEST_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_BULK_WRITE_TEST_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_DEBUG;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_ERASE_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_ERASE_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_ERROR;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_GET_VERSION_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_GET_VERSION_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_MEASURE_SPEED_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_MEASURE_SPEED_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_MEASURE_VOLTAGES_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_MEASURE_VOLTAGES_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_READ_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_READ_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_RECALIBRATE_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_RECALIBRATE_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_SEEK_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_SEEK_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_SET_DRIVE_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_SET_DRIVE_REPLY;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_WRITE_CMD;
import static com.cowlark.fluxengine.external.FluxEngine.F_FRAME_WRITE_REPLY;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import javax.usb.UsbConfiguration;
import javax.usb.UsbEndpoint;
import javax.usb.UsbException;
import javax.usb.UsbInterface;
import javax.usb.UsbPipe;
import java.util.List;

/**
 * FluxEngine floppy drive device, ported from lib/usb/fluxengineusb.cc.
 */
class FluxEngineUsbDevice extends UsbDevice
{
    private static final int MAX_TRANSFER = 32 * 1024;

    private final javax.usb.UsbDevice device;
    private final UsbInterface usbInterface;
    private final UsbPipe cmdOut;
    private final UsbPipe cmdIn;
    private final UsbPipe dataOut;
    private final UsbPipe dataIn;
    private final byte[] buffer = new byte[FRAME_SIZE];

    FluxEngineUsbDevice(javax.usb.UsbDevice device)
    {
        this.device = device;

        UsbInterface iface = null;
        try
        {
            for (Object o : device.getUsbConfigurations())
            {
                UsbConfiguration config = (UsbConfiguration) o;
                for (Object i : config.getUsbInterfaces())
                {
                    UsbInterface candidate = (UsbInterface) i;
                    if (candidate.getUsbEndpoints().size() >= 4)
                        iface = candidate;
                }
            }
            if (iface == null)
                throw new FluxEngineException("FluxEngine: no suitable USB interface found");

            iface.claim();
            usbInterface = iface;

            List<UsbEndpoint> endpoints = iface.getUsbEndpoints();
            UsbPipe cOut = null;
            UsbPipe cIn = null;
            UsbPipe dOut = null;
            UsbPipe dIn = null;
            for (UsbEndpoint endpoint : endpoints)
            {
                int address = endpoint.getUsbEndpointDescriptor().bEndpointAddress() & 0xff;
                UsbPipe pipe = endpoint.getUsbPipe();
                pipe.open();
                switch (address)
                {
                    case FLUXENGINE_CMD_OUT_EP:
                        cOut = pipe;
                        break;
                    case FLUXENGINE_CMD_IN_EP:
                        cIn = pipe;
                        break;
                    case FLUXENGINE_DATA_OUT_EP:
                        dOut = pipe;
                        break;
                    case FLUXENGINE_DATA_IN_EP:
                        dIn = pipe;
                        break;
                }
            }
            if (cOut == null || cIn == null || dOut == null || dIn == null)
                throw new FluxEngineException("FluxEngine: could not open all USB pipes");
            cmdOut = cOut;
            cmdIn = cIn;
            dataOut = dOut;
            dataIn = dIn;
        } catch (UsbException e)
        {
            throw new FluxEngineException("FluxEngine: USB error: " + e.getMessage());
        }

        int version = getVersion();
        if (version != FLUXENGINE_PROTOCOL_VERSION)
            throw new FluxEngineException(String.format(
                    "your FluxEngine firmware is at version %d but the client is for version %d; " +
                            "please upgrade", version, FLUXENGINE_PROTOCOL_VERSION));
    }

    private static double getCurrentTime()
    {
        return System.nanoTime() / 1e9;
    }

    private static Voltages readVoltages(byte[] r, int ptr)
    {
        int logic0 = (r[ptr] & 0xff) | ((r[ptr + 1] & 0xff) << 8);
        int logic1 = (r[ptr + 2] & 0xff) | ((r[ptr + 3] & 0xff) << 8);
        return new Voltages(logic0, logic1);
    }

    private void usbCmdSend(byte[] data)
    {
        try
        {
            cmdOut.syncSubmit(data);
        } catch (UsbException e)
        {
            throw new FluxEngineException("FluxEngine: command send failed: " + e.getMessage());
        }
    }

    private byte[] usbCmdRecv(int len)
    {
        byte[] data = new byte[len];
        try
        {
            cmdIn.syncSubmit(data);
        } catch (UsbException e)
        {
            throw new FluxEngineException("FluxEngine: command recv failed: " + e.getMessage());
        }
        return data;
    }

    private void usbDataSend(Bytes bytes)
    {
        int ptr = 0;
        while (ptr < bytes.size())
        {
            int len = Math.min(bytes.size() - ptr, MAX_TRANSFER);
            byte[] data = new byte[len];
            for (int i = 0; i < len; i++)
                data[i] = bytes.getByte(ptr + i);
            try
            {
                dataOut.syncSubmit(data);
            } catch (UsbException e)
            {
                throw new FluxEngineException("FluxEngine: data send failed: " + e.getMessage());
            }
            ptr += len;
        }
    }

    private Bytes usbDataRecv(int maxLength)
    {
        Bytes bytes = new Bytes(0);
        ByteWriter bw = bytes.writer();
        int ptr = 0;
        while (ptr < maxLength)
        {
            int len = Math.min(maxLength - ptr, MAX_TRANSFER);
            byte[] data = new byte[len];
            int transferred;
            try
            {
                transferred = dataIn.syncSubmit(data);
            } catch (UsbException e)
            {
                throw new FluxEngineException("FluxEngine: data recv failed: " + e.getMessage());
            }
            for (int i = 0; i < transferred; i++)
                bw.write8(data[i] & 0xff);
            ptr += transferred;
            if (transferred < MAX_TRANSFER)
                break;
        }
        return bytes;
    }

    private void badReply()
    {
        int type = buffer[0] & 0xff;
        if (type != F_FRAME_ERROR)
            throw new FluxEngineException(String.format("bad USB reply 0x%2x", type));
        switch (buffer[1] & 0xff)
        {
            case F_ERROR_BAD_COMMAND:
                throw new FluxEngineException("device did not understand command");

            case F_ERROR_UNDERRUN:
                throw new FluxEngineException("USB underrun (not enough bandwidth)");

            default:
                throw new FluxEngineException("unknown device error " + (buffer[1] & 0xff));
        }
    }

    private byte[] awaitReply(int desired)
    {
        for (; ; )
        {
            byte[] r = usbCmdRecv(FRAME_SIZE);
            System.arraycopy(r, 0, buffer, 0, FRAME_SIZE);
            int type = r[0] & 0xff;
            if (type == F_FRAME_DEBUG)
            {
                /* The debug payload is a NUL-terminated string. */
                StringBuilder sb = new StringBuilder();
                for (int i = 2; i < r.length && r[i] != 0; i++)
                    sb.append((char) r[i]);
                System.out.println("dev: " + sb);
                continue;
            }
            if (type != desired)
                badReply();
            return r;
        }
    }

    private int getVersion()
    {
        byte[] f = {F_FRAME_GET_VERSION_CMD, 2};
        usbCmdSend(f);
        byte[] r = awaitReply(F_FRAME_GET_VERSION_REPLY);
        return r[2] & 0xff;
    }

    @Override
    public void seek(int track)
    {
        byte[] f = {F_FRAME_SEEK_CMD, 3, (byte) track};
        usbCmdSend(f);
        awaitReply(F_FRAME_SEEK_REPLY);
    }

    @Override
    public void recalibrate()
    {
        byte[] f = {F_FRAME_RECALIBRATE_CMD, 2};
        usbCmdSend(f);
        awaitReply(F_FRAME_RECALIBRATE_REPLY);
    }

    @Override
    public double getRotationalPeriod(int hardSectorCount)
    {
        byte[] f = {F_FRAME_MEASURE_SPEED_CMD, 3, (byte) hardSectorCount};
        usbCmdSend(f);

        byte[] r = awaitReply(F_FRAME_MEASURE_SPEED_REPLY);
        int periodMs = (r[2] & 0xff) | ((r[3] & 0xff) << 8);
        return periodMs * 1000000.0;
    }

    @Override
    public void testBulkWrite()
    {
        byte[] f = {F_FRAME_BULK_WRITE_TEST_CMD, 2};
        usbCmdSend(f);

        /* These must match the device. */
        final int XSIZE = 64;
        final int YSIZE = 256;
        final int ZSIZE = 64;

        System.out.print("Reading data: ");
        System.out.flush();
        double startTime = getCurrentTime();
        Bytes bulkBuffer = usbDataRecv(XSIZE * YSIZE * ZSIZE);
        double elapsedTime = getCurrentTime() - startTime;

        System.out.println("transferred " + bulkBuffer.size() + " bytes from device -> PC in " +
                (int) (elapsedTime * 1000.0) + " ms (" +
                (int) ((bulkBuffer.size() / 1024.0) / elapsedTime) + " kB/s)");

        for (int x = 0; x < XSIZE; x++)
            for (int y = 0; y < YSIZE; y++)
                for (int z = 0; z < ZSIZE; z++)
                {
                    int offset = x * XSIZE * YSIZE + y * ZSIZE + z;
                    if ((bulkBuffer.getByte(offset) & 0xff) != (x + y + z) % 256)
                        throw new FluxEngineException(String.format(
                                "data transfer corrupted at " + "0x%x %d.%d.%d",
                                offset,
                                x,
                                y,
                                z));
                }

        awaitReply(F_FRAME_BULK_WRITE_TEST_REPLY);
    }

    @Override
    public void testBulkRead()
    {
        byte[] f = {F_FRAME_BULK_READ_TEST_CMD, 2};
        usbCmdSend(f);

        /* These must match the device. */
        final int XSIZE = 64;
        final int YSIZE = 256;
        final int ZSIZE = 64;

        Bytes bulkBuffer = new Bytes(XSIZE * YSIZE * ZSIZE);
        for (int x = 0; x < XSIZE; x++)
            for (int y = 0; y < YSIZE; y++)
                for (int z = 0; z < ZSIZE; z++)
                {
                    int offset = x * XSIZE * YSIZE + y * ZSIZE + z;
                    bulkBuffer.setByte(offset, (byte) (x + y + z));
                }

        System.out.print("Writing data: ");
        System.out.flush();
        double startTime = getCurrentTime();
        usbDataSend(bulkBuffer);
        double elapsedTime = getCurrentTime() - startTime;

        System.out.println("transferred " + bulkBuffer.size() + " bytes from PC -> device in " +
                (int) (elapsedTime * 1000.0) + " ms (" +
                (int) ((bulkBuffer.size() / 1024.0) / elapsedTime) + " kB/s)");

        awaitReply(F_FRAME_BULK_READ_TEST_REPLY);
    }

    @Override
    public Bytes read(int side, boolean synced, double readTimeNs, double hardSectorThresholdNs)
    {
        Bytes f = new Bytes(0);
        ByteWriter bw = f.writer();
        bw.write8(F_FRAME_READ_CMD);
        bw.write8(6);
        bw.write8(side);
        bw.write8(synced ? 1 : 0);
        int milliseconds = (int) (readTimeNs / 1e6);
        bw.write8(milliseconds & 0xff);
        bw.write8((milliseconds >> 8) & 0xff);
        bw.write8((int) ((hardSectorThresholdNs + 5e5) / 1e6)); /* round to nearest ms */
        usbCmdSend(f.toByteArray());

        Bytes buffer = usbDataRecv(1024 * 1024);

        awaitReply(F_FRAME_READ_REPLY);
        return buffer;
    }

    @Override
    public void write(int side, Bytes bytes, double hardSectorThresholdNs)
    {
        int safelen = bytes.size() & ~(FRAME_SIZE - 1);
        Bytes safeBytes = bytes.slice(0, safelen);

        Bytes f = new Bytes(0);
        ByteWriter bw = f.writer();
        bw.write8(F_FRAME_WRITE_CMD);
        bw.write8(7);
        bw.write8(side);
        bw.write8(safelen & 0xff);
        bw.write8((safelen >> 8) & 0xff);
        bw.write8((safelen >> 16) & 0xff);
        bw.write8((safelen >> 24) & 0xff);
        bw.write8((int) ((hardSectorThresholdNs + 5e5) / 1e6)); /* round to nearest ms */
        usbCmdSend(f.toByteArray());
        usbDataSend(safeBytes);

        awaitReply(F_FRAME_WRITE_REPLY);
    }

    @Override
    public void erase(int side, double hardSectorThresholdNs)
    {
        Bytes f = new Bytes(0);
        ByteWriter bw = f.writer();
        bw.write8(F_FRAME_ERASE_CMD);
        bw.write8(3);
        bw.write8(side);
        bw.write8((int) ((hardSectorThresholdNs + 5e5) / 1e6)); /* round to nearest ms */
        usbCmdSend(f.toByteArray());

        awaitReply(F_FRAME_ERASE_REPLY);
    }

    @Override
    public void setDrive(int drive, boolean highDensity, int indexMode)
    {
        byte[] f = {F_FRAME_SET_DRIVE_CMD,
                5,
                (byte) drive,
                (byte) (highDensity ? 1 : 0),
                (byte) indexMode};
        usbCmdSend(f);
        awaitReply(F_FRAME_SET_DRIVE_REPLY);
    }

    @Override
    public VoltageMeasurements measureVoltages()
    {
        byte[] f = {F_FRAME_MEASURE_VOLTAGES_CMD, 2};
        usbCmdSend(f);

        byte[] r = awaitReply(F_FRAME_MEASURE_VOLTAGES_REPLY);

        VoltageMeasurements measurements = new VoltageMeasurements();
        int ptr = 2;
        measurements.outputBothOff = readVoltages(r, ptr);
        ptr += 4;
        measurements.outputDrive0Selected = readVoltages(r, ptr);
        ptr += 4;
        measurements.outputDrive1Selected = readVoltages(r, ptr);
        ptr += 4;
        measurements.outputDrive0Running = readVoltages(r, ptr);
        ptr += 4;
        measurements.outputDrive1Running = readVoltages(r, ptr);
        ptr += 4;
        measurements.inputBothOff = readVoltages(r, ptr);
        ptr += 4;
        measurements.inputDrive0Selected = readVoltages(r, ptr);
        ptr += 4;
        measurements.inputDrive1Selected = readVoltages(r, ptr);
        ptr += 4;
        measurements.inputDrive0Running = readVoltages(r, ptr);
        ptr += 4;
        measurements.inputDrive1Running = readVoltages(r, ptr);
        return measurements;
    }

    @Override
    public void close()
    {
        try
        {
            if (usbInterface.isClaimed())
                usbInterface.release();
        } catch (UsbException e)
        {
            throw new FluxEngineException("FluxEngine: USB error: " + e.getMessage());
        }
    }
}
