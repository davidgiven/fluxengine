package com.cowlark.fluxengine.usb;

import static com.cowlark.fluxengine.usb.LibUsbHelper.check;
import static com.cowlark.fluxengine.usb.LibUsbHelper.checkReset;
import static com.cowlark.fluxengine.wiring.FluxEngine.FLUXENGINE_CMD_IN_EP;
import static com.cowlark.fluxengine.wiring.FluxEngine.FLUXENGINE_CMD_OUT_EP;
import static com.cowlark.fluxengine.wiring.FluxEngine.FLUXENGINE_DATA_IN_EP;
import static com.cowlark.fluxengine.wiring.FluxEngine.FLUXENGINE_DATA_OUT_EP;
import static com.cowlark.fluxengine.wiring.FluxEngine.FLUXENGINE_PROTOCOL_VERSION;
import static com.cowlark.fluxengine.wiring.FluxEngine.FRAME_SIZE;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_ERROR_BAD_COMMAND;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_ERROR_INTERNAL;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_ERROR_INVALID_VALUE;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_ERROR_UNDERRUN;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_BULK_READ_TEST_CMD;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_BULK_READ_TEST_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_BULK_WRITE_TEST_CMD;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_BULK_WRITE_TEST_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_DEBUG;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_ERASE_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_ERROR;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_GET_VERSION_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_MEASURE_SPEED_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_MEASURE_VOLTAGES_CMD;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_MEASURE_VOLTAGES_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_READ_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_SEEK_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_SET_DRIVE_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.F_FRAME_WRITE_REPLY;
import static com.cowlark.fluxengine.wiring.FluxEngine.ReadFrame;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.wiring.FluxEngine.AnyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.EraseFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.MeasureSpeedFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.MeasureSpeedReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.SeekFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.SetDriveFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VersionFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VersionReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VoltagesReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.WriteFrame;
import org.indunet.fastproto.FastProto;
import org.slf4j.LoggerFactory;
import org.usb4java.Device;
import org.usb4java.DeviceHandle;
import org.usb4java.LibUsb;
import java.nio.ByteBuffer;
import java.nio.IntBuffer;

class FluxEngineUsbDevice extends UsbDevice
{
    private static final org.slf4j.Logger logger =
            LoggerFactory.getLogger(FluxEngineUsbDevice.class);

    private static final int MAX_TRANSFER = 32 * 1024;
    private static final long TIMEOUT_MS = 5000;

    private final Device device;
    private final ConfigProto config;
    private final DeviceHandle handle;
    private final byte[] buffer = new byte[FRAME_SIZE];
    private boolean closed = false;

    FluxEngineUsbDevice(Device device, ConfigProto config)
    {
        this.device = device;
        this.config = config;
        this.handle = new DeviceHandle();

        boolean handleOpened = false;
        boolean interfaceClaimed = false;
        try
        {
            check(LibUsb.open(device, handle), "FluxEngine: USB open failed");
            handleOpened = true;

            /* Try to enable auto-detach of kernel driver where supported. */
            int autoDetachRc = LibUsb.setAutoDetachKernelDriver(handle, true);
            if (autoDetachRc != LibUsb.SUCCESS && autoDetachRc != LibUsb.ERROR_NOT_SUPPORTED &&
                    autoDetachRc != LibUsb.ERROR_NOT_FOUND)
            {
                logger
                        .atDebug()
                        .log(
                                "setAutoDetachKernelDriver failed: {}",
                                LibUsb.errorName(autoDetachRc));
            }

            /* Ensure configuration 1 is active (mirrors libusb_set_configuration
             * in lib/usb/usb.cc). If getConfiguration fails we still try to set
             * it; a missing configuration typically means the Zadig driver is not
             * installed. */
            IntBuffer cfg = IntBuffer.allocate(1);
            switch (LibUsb.getConfiguration(handle, cfg))
            {
                case LibUsb.SUCCESS ->
                {
                    if (cfg.get(0) != 1)
                    {
                        check(
                                LibUsb.setConfiguration(handle, 1),
                                "FluxEngine: setConfiguration failed");
                    }
                }

                case LibUsb.ERROR_NOT_FOUND, LibUsb.ERROR_NO_DEVICE ->
                        throw new FluxEngineException("You need to install the Zadig driver");

                default ->
                {
                    /* Best-effort: try to set configuration 1 anyway. */
                    int rc = LibUsb.setConfiguration(handle, 1);
                    if (rc != LibUsb.SUCCESS && rc != LibUsb.ERROR_BUSY)
                        logger
                                .atDebug()
                                .log("setConfiguration(1) returned {}", LibUsb.errorName(rc));
                }
            }

            logger.atDebug().log("claiming USB interface 0");
            check(
                    LibUsb.claimInterface(handle, 0),
                    "FluxEngine: claimInterface failed");
            interfaceClaimed = true;

            logger.atDebug().log("resetting USB device");
            checkReset(LibUsb.resetDevice(handle));

            int version = getVersion();
            if (version != FLUXENGINE_PROTOCOL_VERSION)
                throw new FluxEngineException(String.format(
                        "your FluxEngine firmware is at version %d but the client is for version " +
                                "%d; please upgrade", version, FLUXENGINE_PROTOCOL_VERSION));
        } catch (RuntimeException e)
        {
            try
            {
                logger.atDebug().setCause(e).log("opening device failed, cleaning up");
                if (interfaceClaimed)
                {
                    int rc = LibUsb.releaseInterface(handle, 0);
                    if (rc != LibUsb.SUCCESS)
                        logger
                                .atWarn()
                                .log(
                                        "releaseInterface failed during cleanup: {}",
                                        LibUsb.errorName(rc));
                }
                if (handleOpened)
                    LibUsb.close(handle);
                LibUsb.unrefDevice(device);
                closed = true;
            } catch (RuntimeException suppressed)
            {
                e.addSuppressed(suppressed);
            }
            throw e;
        }
    }

    @Override
    public void close()
    {
        if (closed)
            return;
        closed = true;
        try
        {
            int rc = LibUsb.releaseInterface(handle, 0);
            if (rc != LibUsb.SUCCESS && rc != LibUsb.ERROR_NO_DEVICE &&
                    rc != LibUsb.ERROR_NOT_FOUND)
            {
                logger.atDebug().log("releaseInterface failed: {}", LibUsb.errorName(rc));
            }
        } finally
        {
            LibUsb.close(handle);
            LibUsb.unrefDevice(device);
        }
    }

    private static double getCurrentTime()
    {
        return System.nanoTime() / 1e9;
    }

    private void usbCmdSend(byte[] data)
    {
        ByteBuffer buf = ByteBuffer.allocateDirect(data.length);
        buf.put(data, 0, data.length);
        buf.rewind();
        IntBuffer transferred = IntBuffer.allocate(1);
        check(
                LibUsb.interruptTransfer(
                        handle,
                        (byte) FLUXENGINE_CMD_OUT_EP,
                        buf,
                        transferred,
                        TIMEOUT_MS),
                "FluxEngine: command send failed");
    }

    private void usbCmdSend(Object object)
    {
        usbCmdSend(FastProto.encode(object));
    }

    private byte[] usbCmdRecv(int len)
    {
        ByteBuffer buf = ByteBuffer.allocateDirect(len);
        IntBuffer transferred = IntBuffer.allocate(1);
        check(
                LibUsb.interruptTransfer(
                        handle,
                        (byte) FLUXENGINE_CMD_IN_EP,
                        buf,
                        transferred,
                        TIMEOUT_MS),
                "FluxEngine: command recv failed");
        int n = transferred.get(0);
        byte[] data = new byte[len];
        buf.rewind();
        buf.get(data, 0, Math.min(n, len));
        /* libusb may return fewer bytes with an interrupt transfer; pad
         * remaining bytes with zero to mimic javax.usb syncSubmit behaviour. */
        return data;
    }

    private void usbDataSend(Bytes bytes)
    {
        int ptr = 0;
        while (ptr < bytes.size())
        {
            int len = Math.min(bytes.size() - ptr, MAX_TRANSFER);
            ByteBuffer buf = ByteBuffer.allocateDirect(len);
            for (int i = 0; i < len; i++)
                buf.put((byte) bytes.getByte(ptr + i));
            buf.rewind();
            IntBuffer transferred = IntBuffer.allocate(1);
            int rc = LibUsb.bulkTransfer(
                    handle,
                    (byte) FLUXENGINE_DATA_OUT_EP,
                    buf,
                    transferred,
                    TIMEOUT_MS);
            check(rc, "FluxEngine: data send failed");
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
            ByteBuffer buf = ByteBuffer.allocateDirect(len);
            IntBuffer transferred = IntBuffer.allocate(1);
            check(
                    LibUsb.bulkTransfer(
                            handle,
                            (byte) FLUXENGINE_DATA_IN_EP,
                            buf,
                            transferred,
                            TIMEOUT_MS),
                    "FluxEngine: data recv failed");
            int n = transferred.get(0);
            buf.rewind();
            for (int i = 0; i < n; i++)
                bw.write8(buf.get() & 0xff);
            ptr += n;
            if (n < len)
                break;
            if (n == 0)
                break;
        }
        return bytes;
    }

    private void badReply()
    {
        int type = buffer[0] & 0xff;
        if (type != F_FRAME_ERROR)
            throw new FluxEngineException(String.format("bad USB reply 0x%2x", type));
        switch (buffer[2] & 0xff)
        {
            case F_ERROR_BAD_COMMAND:
                throw new FluxEngineException("device did not understand command");

            case F_ERROR_UNDERRUN:
                throw new UsbUnderrunException();

            case F_ERROR_INVALID_VALUE:
                throw new FluxEngineException("device received a bad parameter");

            case F_ERROR_INTERNAL:
                throw new FluxEngineException("device experienced an internal error");

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
                logger.atDebug().log("dev: {}", sb);
                continue;
            }
            if (type != desired)
                badReply();
            return r;
        }
    }

    private <T> T awaitReply(int desired, Class<T> dataClass)
    {
        byte[] bytes = awaitReply(desired);
        return FastProto.decode(bytes, dataClass);
    }

    private int getVersion()
    {
        usbCmdSend(new VersionFrame());
        VersionReplyFrame reply = awaitReply(F_FRAME_GET_VERSION_REPLY, VersionReplyFrame.class);
        return reply.getVersion();
    }

    @Override
    public void seek(int cylinder)
    {
        usbCmdSend(SetDriveFrame
                .builder()
                .setDrive(config.getDrive().getDrive())
                .setHighDensity(config.getDrive().getHighDensity() ? 1 : 0)
                .setIndexMode(0)
                .build());
        awaitReply(F_FRAME_SET_DRIVE_REPLY);

        usbCmdSend(SeekFrame.builder().setTrack(cylinder).build());
        awaitReply(F_FRAME_SEEK_REPLY);
    }

    @Override
    public double getRotationalPeriod()
    {
        usbCmdSend(MeasureSpeedFrame
                .builder()
                .setHardSectorCount(config.getDrive().getHardSectorCount())
                .build());

        MeasureSpeedReplyFrame r =
                awaitReply(F_FRAME_MEASURE_SPEED_REPLY, MeasureSpeedReplyFrame.class);
        return r.getPeriodMs() * 1000000.0;
    }

    @Override
    public void testBulkWrite()
    {
        usbCmdSend(AnyFrame.builder().setType(F_FRAME_BULK_WRITE_TEST_CMD).setSize(2).build());

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
        usbCmdSend(AnyFrame.builder().setType(F_FRAME_BULK_READ_TEST_CMD).setSize(2).build());

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
    public Bytes read(int cylinder, int head, double readTimeNs)
    {
        for (int i = 0; i < config.getUsb().getFluxengine().getUnderrunRetries(); i++)
        {
            try
            {
                seek(cylinder);

                usbCmdSend(ReadFrame
                        .builder()
                        .setSide(head)
                        .setSynced(config.getDrive().getSyncWithIndex() ? 1 : 0)
                        .setMilliseconds((int) (readTimeNs / 1e6))
                        .setHardsecThresholdMs((int) (
                                        (config.getDrive().getHardSectorThresholdNs() + 5e5) / 1e6)
                                /* round to nearest ms */)
                        .build());

                Bytes buffer = usbDataRecv(1024 * 1024);

                awaitReply(F_FRAME_READ_REPLY);
                return buffer;
            } catch (UsbUnderrunException e)
            {
                logger.atInfo().log("USB underrun, retrying");
            }
        }
        throw new FluxEngineException(
                "Consistent USB underruns --- you don't have enough " + "bandwidth");
    }

    @Override
    public void write(int cylinder, int head, Bytes bytes)
    {
        for (int i = 0; i < config.getUsb().getFluxengine().getUnderrunRetries(); i++)
        {
            try
            {
                seek(cylinder);

                int safelen = bytes.size() & ~(FRAME_SIZE - 1);
                Bytes safeBytes = bytes.slice(0, safelen);

                usbCmdSend(WriteFrame
                        .builder()
                        .setSide(head)
                        .setBytesToWrite(safelen)
                        .setHardsecThresholdMs((int) (
                                        (config.getDrive().getHardSectorThresholdNs() + 5e5) / 1e6)
                                /* round to nearest ms */)
                        .build());
                usbDataSend(safeBytes);

                awaitReply(F_FRAME_WRITE_REPLY);
            } catch (UsbUnderrunException e)
            {
                logger.atInfo().log("USB underrun, retrying");
            }
        }
        throw new FluxEngineException(
                "Consistent USB underruns --- you don't have enough " + "bandwidth");
    }

    @Override
    public void erase(int cylinder, int head)
    {
        seek(cylinder);

        usbCmdSend(EraseFrame
                .builder()
                .setSide(head)
                .setHardsecThresholdMs((int) ((config.getDrive().getHardSectorThresholdNs() + 5e5) /
                        1e6))
                .build());

        awaitReply(F_FRAME_ERASE_REPLY);
    }

    @Override
    public VoltagesReplyFrame measureVoltages()
    {
        usbCmdSend(AnyFrame.builder().setType(F_FRAME_MEASURE_VOLTAGES_CMD).setSize(2).build());

        return awaitReply(F_FRAME_MEASURE_VOLTAGES_REPLY, VoltagesReplyFrame.class);
    }
}
