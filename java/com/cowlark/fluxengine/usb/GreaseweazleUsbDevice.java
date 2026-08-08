package com.cowlark.fluxengine.usb;

import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_BAD_COMMAND;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_BAD_CYLINDER;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_BAD_PIN;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_BAD_UNIT;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_FLUX_OVERFLOW;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_FLUX_UNDERFLOW;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_NO_BUS;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_NO_INDEX;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_NO_TRK0;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_NO_UNIT;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_OKAY;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.ACK_WRPROT;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.BAUD_CLEAR_COMMS;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.BAUD_NORMAL;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_ERASE_FLUX;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_GET_FLUX_STATUS;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_GET_INFO;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_HEAD;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_MOTOR;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_READ_FLUX;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SEEK;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SELECT;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SET_BUS_TYPE;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SET_PIN;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SINK_BYTES;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_SOURCE_BYTES;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.CMD_WRITE_FLUX;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.FLUXOP_INDEX;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.FLUXOP_SPACE;
import static com.cowlark.fluxengine.external.GreaseweazleUtils.GETINFO_FIRMWARE;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.external.GreaseweazleUtils;
import com.fazecast.jSerialComm.SerialPort;
import com.google.common.util.concurrent.Uninterruptibles;
import java.time.Duration;

/**
 * Greaseweazle floppy drive device, ported from lib/usb/greaseweazleusb.cc.
 */
class GreaseweazleUsbDevice extends UsbDevice
{
    private final SerialPort serial;
    private final GreaseweazleProto config;
    private Version version;
    private long clock;
    private long revolutions;

    GreaseweazleUsbDevice(String port, GreaseweazleProto config)
    {
        this.config = config;
        this.serial = SerialPort.getCommPort(port);
        serial.setBaudRate(BAUD_NORMAL);
        serial.setComPortTimeouts(SerialPort.TIMEOUT_READ_SEMI_BLOCKING, 0, 0);
        if (!serial.openPort())
            throw new FluxEngineException("Unable to open serial port " + port);

        int version = getVersion();
        if (version >= 29)
            this.version = Version.V29;
        else if (version >= 24)
            this.version = Version.V24;
        else if (version == 22)
            this.version = Version.V22;
        else
            throw new FluxEngineException(String.format(
                    "only Greaseweazle firmware versions 22 and 24 or above are currently " +
                            "supported, but you have version %d. Please file a bug.", version));

        /* Twiddle the baud rate, which indicates to the Greaseweazle that the
         * data stream has been reset. */
        serial.setBaudRate(BAUD_CLEAR_COMMS);
        Uninterruptibles.sleepUninterruptibly(Duration.ofMillis(100));
        serial.setBaudRate(BAUD_NORMAL);

        /* Configure the hardware. */
        doCommand(CMD_SET_BUS_TYPE, config.getBusType().getNumber());
    }

    private static String gwError(int e)
    {
        switch (e)
        {
            case ACK_OKAY:
                return "OK";
            case ACK_BAD_COMMAND:
                return "Bad command";
            case ACK_NO_INDEX:
                return "No index";
            case ACK_NO_TRK0:
                return "No track 0";
            case ACK_FLUX_OVERFLOW:
                return "Overflow";
            case ACK_FLUX_UNDERFLOW:
                return "Underflow";
            case ACK_WRPROT:
                return "Write protected";
            case ACK_NO_UNIT:
                return "No unit";
            case ACK_NO_BUS:
                return "No bus";
            case ACK_BAD_UNIT:
                return "Invalid unit";
            case ACK_BAD_PIN:
                return "Invalid pin";
            case ACK_BAD_CYLINDER:
                return "Invalid track";
            default:
                return "Unknown error";
        }
    }

    private static long ssRandNext(long x)
    {
        return (x & 1) != 0 ? (x >> 1) ^ 0x80000062L : x >> 1;
    }

    private static double getCurrentTime()
    {
        return System.nanoTime() / 1e9;
    }

    private int getVersion()
    {
        doCommand(CMD_GET_INFO, GETINFO_FIRMWARE);

        ByteReader response = new ByteReader(readBytes(32));
        response.seek(4);
        long freq = response.readLe32() & 0xffffffffL;
        clock = 1000000000L / freq;

        response.seek(0);
        return response.readBe16();
    }

    private long read28()
    {
        ByteReader buffer = new ByteReader(readBytes(4));
        return (long) ((buffer.read8() & 0xfe) >> 1) | (long) (buffer.read8() & 0xfe) << 6 |
                (long) (buffer.read8() & 0xfe) << 13 | (long) (buffer.read8() & 0xfe) << 20;
    }

    private void doCommand(int cmd, int... payload)
    {
        byte[] command = new byte[2 + payload.length];
        command[0] = (byte) cmd;
        command[1] = (byte) command.length;
        for (int i = 0; i < payload.length; i++)
            command[2 + i] = (byte) payload[i];
        doCommand(command);
    }

    private void doCommand(Bytes command)
    {
        doCommand(command.toByteArray());
    }

    private void doCommand(byte[] command)
    {
        writeBytes(command);

        Bytes buffer = readBytes(2);

        if ((buffer.getByte(0) & 0xff) != (command[0] & 0xff))
            throw new FluxEngineException(String.format(
                    "command returned garbage (0x%x != 0x%x with status 0x%x)",
                    buffer.getByte(0),
                    command[0],
                    buffer.getByte(1)));
        if (buffer.getByte(1) != 0)
            throw new FluxEngineException(
                    "Greaseweazle error: " + gwError(buffer.getByte(1) & 0xff));
    }

    @Override
    public void seek(int track)
    {
        doCommand(CMD_SEEK, track);
    }

    @Override
    public double getRotationalPeriod(int hardSectorCount)
    {
        if (hardSectorCount != 0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Greaseweazle");

        /* The Greaseweazle doesn't have a command to fetch the period directly,
         * so we have to do a flux read. */
        switch (version)
        {
            case V22:
                doCommand(CMD_READ_FLUX);
                break;

            case V24:
            case V29:
            {
                Bytes cmd = new Bytes(0);
                ByteWriter bw = new ByteWriter(cmd);
                bw.write8(CMD_READ_FLUX);
                bw.write8(8);
                bw.writeLe32(0);  /* ticks default value (guessed) */
                bw.writeLe16(2); /* revolutions */
                doCommand(cmd);
            }
        }

        long ticksGw = 0;
        long firstIndex = -1;
        long secondIndex = -1;
        for (; ; )
        {
            int b = readByte();
            if (b == 0)
                break;

            if (b == 255)
            {
                switch (readByte())
                {
                    case FLUXOP_INDEX:
                    {
                        long index = read28() + ticksGw;
                        if (firstIndex == -1)
                            firstIndex = index;
                        else if (secondIndex == -1)
                            secondIndex = index;
                        break;
                    }

                    case FLUXOP_SPACE:
                        ticksGw += read28();
                        break;

                    default:
                        throw new FluxEngineException("bad opcode in Greaseweazle stream");
                }
            } else
            {
                if (b < 250)
                    ticksGw += b;
                else
                {
                    long delta = 250 + (b - 250) * 255 + readByte() - 1;
                    ticksGw += delta;
                }
            }
        }

        if (secondIndex == -1)
            throw new FluxEngineException(
                    "unable to determine disk rotational period (is a disk in the drive?)");
        doCommand(CMD_GET_FLUX_STATUS);

        revolutions = (secondIndex - firstIndex) * clock;
        return revolutions;
    }

    @Override
    public void testBulkWrite()
    {
        System.out.print("Writing data: ");
        final int LEN = 10 * 1024 * 1024;
        Bytes cmd = new Bytes(0);
        ByteWriter bw = new ByteWriter(cmd);
        switch (version)
        {
            case V22:
            case V24:
                bw.write8(CMD_SINK_BYTES);
                bw.write8(6);
                bw.writeLe32(LEN);
                break;

            case V29:
                bw.write8(CMD_SINK_BYTES);
                bw.write8(10);
                bw.writeLe32(LEN);
                bw.writeLe32(0); /* seed */
                break;

            default:
                throw new IllegalStateException();
        }
        doCommand(cmd);

        Bytes junk = new Bytes(0);
        ByteWriter jw = new ByteWriter(junk);
        long seed = 0;
        for (int i = 0; i < LEN; i++)
        {
            jw.write8((int) seed);
            seed = ssRandNext(seed);
        }
        double startTime = getCurrentTime();
        writeBytes(junk);
        readBytes(1);
        double elapsedTime = getCurrentTime() - startTime;

        System.out.printf(
                "transferred %d bytes from PC -> device in %d ms (%d kb/s)\n",
                LEN,
                (int) (elapsedTime * 1000.0),
                (int) ((LEN / 1024.0) / elapsedTime));
    }

    @Override
    public void testBulkRead()
    {
        System.out.print("Reading data: ");
        final int LEN = 10 * 1024 * 1024;
        Bytes cmd = new Bytes(0);
        ByteWriter bw = new ByteWriter(cmd);
        switch (version)
        {
            case V22:
            case V24:
                bw.write8(CMD_SOURCE_BYTES);
                bw.write8(6);
                bw.writeLe32(LEN);
                break;

            case V29:
                bw.write8(CMD_SOURCE_BYTES);
                bw.write8(10);
                bw.writeLe32(LEN);
                bw.writeLe32(0); /* seed */
                break;

            default:
                throw new IllegalStateException();
        }
        doCommand(cmd);

        double startTime = getCurrentTime();
        readBytes(LEN);
        double elapsedTime = getCurrentTime() - startTime;

        System.out.printf(
                "transferred %d bytes from device -> PC in %d ms (%d kb/s)\n",
                LEN,
                (int) (elapsedTime * 1000.0),
                (int) ((LEN / 1024.0) / elapsedTime));
    }

    @Override
    public Bytes read(int side, boolean synced, double readTimeNs, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Greaseweazle");

        doCommand(CMD_HEAD, side);

        switch (version)
        {
            case V22:
            {
                long revs = (long) ((readTimeNs + revolutions - 1) / revolutions);
                Bytes cmd = new Bytes(0);
                ByteWriter bw = new ByteWriter(cmd);
                bw.write8(CMD_READ_FLUX);
                bw.write8(4);
                bw.writeLe32((int) (revs + (synced ? 1 : 0)));
                doCommand(cmd);
                break;
            }

            case V24:
            case V29:
            {
                Bytes cmd = new Bytes(0);
                ByteWriter bw = new ByteWriter(cmd);
                bw.write8(CMD_READ_FLUX);
                bw.write8(8);
                bw.writeLe32((int) ((readTimeNs + (synced ? revolutions : 0)) / clock));
                bw.writeLe16(0);
                doCommand(cmd);
            }
        }

        Bytes buffer = new Bytes(0);
        ByteWriter bw = new ByteWriter(buffer);
        for (; ; )
        {
            int b = readByte();
            if (b == 0)
                break;
            bw.write8(b);
        }

        doCommand(CMD_GET_FLUX_STATUS);

        Bytes fldata = GreaseweazleUtils.greaseweazleToFluxEngine(buffer, clock);
        if (synced)
            fldata = GreaseweazleUtils.stripPartialRotation(fldata);
        return fldata;
    }

    @Override
    public void write(int side, Bytes fldata, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Greaseweazle");

        doCommand(CMD_HEAD, side);
        switch (version)
        {
            case V22:
                doCommand(CMD_WRITE_FLUX, 1);
                break;

            case V24:
            case V29:
                doCommand(CMD_WRITE_FLUX, 1, 1);
                break;
        }
        Bytes gwdata = GreaseweazleUtils.fluxEngineToGreaseweazle(fldata, clock);
        writeBytes(gwdata);
        readByte(); /* synchronise */

        doCommand(CMD_GET_FLUX_STATUS);
    }

    @Override
    public void erase(int side, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Greaseweazle");

        doCommand(CMD_HEAD, side);

        Bytes cmd = new Bytes(0);
        ByteWriter bw = new ByteWriter(cmd);
        bw.write8(CMD_ERASE_FLUX);
        bw.write8(6);
        bw.writeLe32((int) (200e6 / clock));
        doCommand(cmd);
        readByte(); /* synchronise */

        doCommand(CMD_GET_FLUX_STATUS);
    }

    @Override
    public void setDrive(int drive, boolean highDensity, int indexMode)
    {
        doCommand(CMD_SELECT, drive);
        doCommand(CMD_MOTOR, drive, 1);
        doCommand(CMD_SET_PIN, 2, highDensity ? 1 : 0);
    }

    @Override
    public VoltageMeasurements measureVoltages()
    {
        throw new FluxEngineException("unsupported operation on the Greaseweazle");
    }

    private int readByte()
    {
        return readBytes(1).get(0) & 0xff;
    }

    private Bytes readBytes(int count)
    {
        Bytes result = new Bytes(0);
        ByteWriter bw = new ByteWriter(result);
        byte[] chunk = new byte[4096];
        while (bw.pos() < count)
        {
            int read = serial.readBytes(chunk, Math.min(chunk.length, count - bw.pos()));
            if (read < 0)
                throw new FluxEngineException("serial read failed");
            for (int i = 0; i < read; i++)
                bw.write8(chunk[i] & 0xff);
        }
        return result;
    }

    private void writeBytes(byte[] data)
    {
        int written = serial.writeBytes(data, data.length);
        if (written != data.length)
            throw new FluxEngineException("serial write failed");
    }

    private void writeBytes(Bytes data)
    {
        writeBytes(data.toByteArray());
    }

    private enum Version
    {V22, V24, V29}
}
