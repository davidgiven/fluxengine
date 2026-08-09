package com.cowlark.fluxengine.usb;

import static com.fazecast.jSerialComm.SerialPort.FLOW_CONTROL_DISABLED;
import static com.fazecast.jSerialComm.SerialPort.TIMEOUT_READ_BLOCKING;
import static com.fazecast.jSerialComm.SerialPort.TIMEOUT_WRITE_BLOCKING;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.fazecast.jSerialComm.SerialPort;
import com.google.common.util.concurrent.Uninterruptibles;
import java.time.Duration;

/**
 * A wrapper around a USB serial port which more closely matches the behaviour
 * of the original lib/usb/serial.c than the raw jSerialComm interface: raw
 * 8N1 mode with no flow control, DTR toggling to reset the device, flushing of
 * pending input on open, and read/write loops which collect or transmit all of
 * the requested bytes.
 */
public final class Serial
{
    private final SerialPort serial;
    private final byte[] readBuffer = new byte[4096];
    private int readBufferPtr = 0;
    private int readBufferFill = 0;

    public Serial(String path, int baudRate)
    {
        serial = SerialPort.getCommPort(path);
        serial.setComPortParameters(baudRate, 8, 1, 0); /* raw 8N1 */
        serial.setFlowControl(FLOW_CONTROL_DISABLED);
        serial.setComPortTimeouts(TIMEOUT_READ_BLOCKING | TIMEOUT_WRITE_BLOCKING, 0, 0);
        if (!serial.openPort())
            throw new FluxEngineException("cannot open serial port '" + path + "'");

        /* Toggle DTR to reset the device. */
        toggleDtr();

        /* Flush pending input from a generic device. */
        readBufferPtr = 0;
        readBufferFill = 0;
    }

    /* Toggles the DTR line, which resets the attached device. The C++ clears
     * DTR, sleeps, and sets it again. */
    public void toggleDtr()
    {
        boolean rts = serial.getRTS();
        serial.setDTRandRTS(false, rts);
        Uninterruptibles.sleepUninterruptibly(Duration.ofMillis(200));
        serial.setDTRandRTS(true, rts);
    }

    public void setBaudRate(int baudRate)
    {
        if (!serial.setBaudRate(baudRate))
            throw new FluxEngineException("cannot set baud rate on serial port");
        toggleDtr();
    }

    public Bytes readBytes(int count)
    {
        byte[] array = new byte[count];
        serial.readBytes(array, count);
        return new Bytes(array);
    }

    public int readByte()
    {
        return readBytes(1).getByte(0);
    }

    public void writeBytes(byte[] data)
    {
        serial.writeBytes(data, data.length);
    }

    public void writeBytes(Bytes data)
    {
        serial.writeBytes(data.toByteArray(), data.size());
    }

    public void writeByte(int b)
    {
        Bytes data = new Bytes(1);
        data.setByte(0, (byte) b);
        writeBytes(data);
    }

    public void writeLine(String s)
    {
        writeBytes(new Bytes(s));
        writeByte('\n');
    }

    public String readLine()
    {
        StringBuilder sb = new StringBuilder();
        for (; ; )
        {
            int b = readByte();
            if (b == '\r')
                continue;
            if (b == '\n')
                return sb.toString();
            sb.append((char) b);
        }
    }

    public void close()
    {
        serial.closePort();
    }
}
