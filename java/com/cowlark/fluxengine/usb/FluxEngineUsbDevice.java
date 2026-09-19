package com.cowlark.fluxengine.usb;

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
import com.cowlark.fluxengine.wiring.FluxEngine;
import com.cowlark.fluxengine.wiring.FluxEngine.AnyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.DebugFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.EraseFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.MeasureSpeedFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.MeasureSpeedReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.SeekFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.SetDriveFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VersionFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VersionReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.VoltagesReplyFrame;
import com.cowlark.fluxengine.wiring.FluxEngine.WriteFrame;
import lombok.SneakyThrows;
import org.indunet.fastproto.FastProto;
import org.slf4j.LoggerFactory;
import javax.usb.UsbConfiguration;
import javax.usb.UsbEndpoint;
import javax.usb.UsbException;
import javax.usb.UsbInterface;
import javax.usb.UsbInterfacePolicy;
import javax.usb.UsbIrp;
import java.util.List;

/**
 * FluxEngine floppy drive device, ported from lib/usb/fluxengineusb.cc.
 */
class FluxEngineUsbDevice extends UsbDevice
{
    private static final org.slf4j.Logger logger =
            LoggerFactory.getLogger(FluxEngineUsbDevice.class);

    private static final int MAX_TRANSFER = 32 * 1024;

    private final javax.usb.UsbDevice device;
    private final ConfigProto config;
    private final UsbInterface usbInterface;
    private final CloseableUsbPipe cmdOut;
    private final CloseableUsbPipe cmdIn;
    private final CloseableUsbPipe dataOut;
    private final CloseableUsbPipe dataIn;
    private final byte[] buffer = new byte[FRAME_SIZE];

    FluxEngineUsbDevice(javax.usb.UsbDevice device, ConfigProto config)
    {
        this.device = device;
        this.config = config;

        try
        {
            try
            {
                UsbConfiguration usbConfig = device.getActiveUsbConfiguration();
                if (usbConfig == null)
                    throw new FluxEngineException("You need to install the Zadig driver");
                usbInterface = usbConfig.getUsbInterface((byte) 0);
                logger.atDebug().log("claiming USB device {}", usbInterface);
                usbInterface.claim((UsbInterfacePolicy) usbInterface -> true);

                List<UsbEndpoint> endpoints = usbInterface.getUsbEndpoints();
                cmdOut = new CloseableUsbPipe(endpoints, FLUXENGINE_CMD_OUT_EP);
                cmdIn = new CloseableUsbPipe(endpoints, FLUXENGINE_CMD_IN_EP);
                dataOut = new CloseableUsbPipe(endpoints, FLUXENGINE_DATA_OUT_EP);
                dataIn = new CloseableUsbPipe(endpoints, FLUXENGINE_DATA_IN_EP);
            } catch (UsbException e)
            {
                throw new FluxEngineException("FluxEngine: USB error: " + e.getMessage());
            }

            int version = getVersion();
            if (version != FLUXENGINE_PROTOCOL_VERSION)
                throw new FluxEngineException(String.format(
                        "your FluxEngine firmware is at version %d but the client is for version " +
                                "%d; " + "please upgrade", version, FLUXENGINE_PROTOCOL_VERSION));
        } catch (RuntimeException e)
        {
            try
            {
                logger.atDebug().setCause(e).log("opening device failed, cleaning up");
                close();
            } catch (RuntimeException suppressed)
            {
                e.addSuppressed(suppressed);
            }
            throw e;
        }
    }

    @Override
    @SneakyThrows
    public void close()
    {
        try
        {
            if (cmdOut != null)
                cmdOut.close();
            if (cmdIn != null)
                cmdIn.close();
            if (dataOut != null)
                dataOut.close();
            if (dataIn != null)
                dataIn.close();
        } catch (UsbException e)
        {
            throw new FluxEngineException(
                    "FluxEngine: USB error closing pipe: " + e.getMessage(),
                    e);
        } finally
        {
            try
            {
                if (usbInterface != null)
                {
                    logger.atDebug().log("releasing USB interface");
                    usbInterface.release();
                }
            } catch (UsbException e)
            {
                throw new FluxEngineException("FluxEngine: USB error: " + e.getMessage(), e);
            }
        }
    }

    private static double getCurrentTime()
    {
        return System.nanoTime() / 1e9;
    }

    private void usbCmdSend(byte[] data)
    {
        try
        {
            cmdOut.get().syncSubmit(data);
        } catch (UsbException e)
        {
            throw new FluxEngineException("FluxEngine: command send failed: " + e.getMessage());
        }
    }

    private void usbCmdSend(Object object)
    {
        usbCmdSend(FastProto.encode(object));
    }

    private byte[] usbCmdRecv(int len)
    {
        byte[] data = new byte[len];
        try
        {
            cmdIn.get().syncSubmit(data);
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
                data[i] = (byte) bytes.getByte(ptr + i);
            try
            {
                dataOut.get().syncSubmit(data);
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
                transferred = dataIn.get().syncSubmit(data);
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
                                (config.getDrive().getHardSectorThresholdNs() + 5e5) /
                                        1e6) /* round to nearest ms */)
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
                                (config.getDrive().getHardSectorThresholdNs() + 5e5) /
                                        1e6) /* round to nearest ms */)
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
