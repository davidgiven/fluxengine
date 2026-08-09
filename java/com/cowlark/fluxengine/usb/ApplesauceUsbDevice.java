package com.cowlark.fluxengine.usb;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.external.FluxEngine.NS_PER_TICK;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.decoders.DecoderProto;
import java.util.ArrayList;
import java.util.List;

/**
 * Applesauce floppy drive device, ported from lib/usb/applesauceusb.cc.
 */
class ApplesauceUsbDevice extends UsbDevice
{
    private final Serial serial;
    private final ApplesauceProto config;
    private boolean connected;

    ApplesauceUsbDevice(String port, ApplesauceProto config)
    {
        this.config = config;
        this.serial = new Serial(port, 9600);

        String s = sendrecv("?");
        if (!s.equals("Applesauce"))
            throw new FluxEngineException(String.format(
                    "Applesauce device not responding " + "(expected 'Applesauce', got '%s')",
                    s));

        doCommand("client:v2");
    }

    private static long ssRandNext(long x)
    {
        return (x & 1) != 0 ? (x >> 1) ^ 0x80000062L : x >> 1;
    }

    private static Bytes applesauceReadDataToFluxEngine(Bytes asdata,
                                                        double clock,
                                                        List<Integer> indexMarks)
    {
        ByteReader br = new ByteReader(asdata);
        Fluxmap fluxmap = new Fluxmap();
        int indexIt = 0;
        fluxmap.appendIndex();

        long totalTicks = 0;
        while (!br.eof())
        {
            int b = br.read8();
            fluxmap.appendInterval((int) (b * clock / NS_PER_TICK));
            if (b != 255)
                fluxmap.appendPulse();

            totalTicks += b;
            if ((indexIt < indexMarks.size()) && (totalTicks > indexMarks.get(indexIt)))
            {
                fluxmap.appendIndex();
                indexIt++;
            }
        }

        return fluxmap.rawBytes();
    }

    private static Bytes fluxEngineToApplesauceWriteData(Bytes fldata)
    {
        Fluxmap fluxmap = new Fluxmap(fldata);
        FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
        Bytes asdata = new Bytes(0);
        ByteWriter bw = asdata.writer();

        while (!fmr.eof())
        {
            FluxmapReader.EventResult r = fmr.findEvent(F_BIT_PULSE);
            long ticks = r.ticks();
            if (!r.found())
                break;

            long applesauceTicks = (long) (ticks * NS_PER_TICK);
            while (applesauceTicks >= 0xffff)
            {
                bw.writeLe16(0xffff);
                applesauceTicks -= 0xffff;
            }
            if (applesauceTicks == 0)
                throw new FluxEngineException("bad data!");
            bw.writeLe16((int) applesauceTicks);
        }

        bw.writeLe16(0);
        return asdata;
    }

    private static double getCurrentTime()
    {
        return System.nanoTime() / 1e9;
    }

    private static List<String> split(String s, char separator)
    {
        List<String> result = new ArrayList<>();
        StringBuilder current = new StringBuilder();
        for (int i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            if (c == separator)
            {
                result.add(current.toString());
                current.setLength(0);
            } else
                current.append(c);
        }
        result.add(current.toString());
        return result;
    }

    private String sendrecv(String command)
    {
        if (config.getVerbose())
            System.out.println("> " + command);
        serial.writeLine(command);
        String r = serial.readLine();
        if (config.getVerbose())
            System.out.println("< " + r);
        return r;
    }

    private void checkCommandResult(String result)
    {
        if (!result.equals("."))
            throw new FluxEngineException("low-level Applesauce error: '" + result + "'");
    }

    private void doCommand(String command)
    {
        checkCommandResult(sendrecv(command));
    }

    private String doCommandX(String command)
    {
        doCommand(command);
        String r = serial.readLine();
        if (config.getVerbose())
            System.out.println("<< " + r);
        return r;
    }

    private void connect()
    {
        if (!connected)
        {
            try
            {
                doCommand("connect");
                doCommand("drive:enable");
                doCommand("motor:on");
                doCommand("head:zero");
                connected = true;
            } catch (FluxEngineException e)
            {
                throw new FluxEngineException("Applesauce could not connect to a drive");
            }
        }
    }

    @Override
    public void seek(int track)
    {
        if (track == 0)
            doCommand("head:zero");
        else
            doCommand(String.format("head:track%d", track));
    }

    @Override
    public double getRotationalPeriod(int hardSectorCount)
    {
        if (hardSectorCount != 0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Applesauce");

        connect();
        try
        {
            double periodUs = Double.parseDouble(doCommandX("sync:?speed"));
            serial.writeByte('X');
            String r = serial.readLine();
            if (config.getVerbose())
                System.out.println("<< " + r);
            return periodUs * 1e3;
        } catch (FluxEngineException e)
        {
            return 0;
        }
    }

    @Override
    public void testBulkWrite()
    {
        int max = Integer.parseInt(sendrecv("data:?max"));
        System.out.print("Writing data: ");

        doCommand(String.format("data:>%d", max));

        Bytes junk = new Bytes(max);
        long seed = 0;
        for (int i = 0; i < max; i++)
        {
            junk.setByte(i, (byte) seed);
            seed = ssRandNext(seed);
        }
        double startTime = getCurrentTime();
        serial.writeBytes(junk);
        serial.readLine();
        double elapsedTime = getCurrentTime() - startTime;

        System.out.printf(
                "transferred %d bytes from PC -> device in %d ms (%d kb/s)%n",
                max,
                (int) (elapsedTime * 1000.0),
                (int) ((max / 1024.0) / elapsedTime));
    }

    @Override
    public void testBulkRead()
    {
        int max = Integer.parseInt(sendrecv("data:?max"));
        System.out.print("Reading data: ");

        doCommand(String.format("data:<%d", max));

        double startTime = getCurrentTime();
        serial.readBytes(max);
        double elapsedTime = getCurrentTime() - startTime;

        System.out.printf(
                "transferred %d bytes from device -> PC in %d ms (%d kb/s)%n",
                max,
                (int) (elapsedTime * 1000.0),
                (int) ((max / 1024.0) / elapsedTime));
    }

    @Override
    public Bytes read(int side, boolean synced, double readTimeNs, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Applesauce");
        boolean shortRead = readTimeNs < 400e6;
        Logger.logf(
                "applesauce: timed reads not supported; using read of %s revolutions",
                shortRead ? "1.25" : "2.25");

        connect();
        doCommand(String.format("head:side%d", side));
        doCommand("sync:on");
        doCommand("data:clear");
        String r = doCommandX(shortRead ? "disk:read" : "disk:readx");
        List<String> rsplit = split(r, '|');
        if (rsplit.size() < 2)
            throw new FluxEngineException(
                    "unrecognised Applesauce response to disk:read: '" + r + "'");

        int bufferSize = Integer.parseInt(rsplit.get(0));
        double tickSize = Double.parseDouble(rsplit.get(1)) / 1e3;

        List<Integer> indexMarks = new ArrayList<>();
        for (int i = 2; i < rsplit.size(); i++)
            indexMarks.add(Integer.parseInt(rsplit.get(i)));

        doCommand(String.format("data:<%d", bufferSize));

        Bytes rawData = serial.readBytes(bufferSize);
        return applesauceReadDataToFluxEngine(rawData, tickSize, indexMarks);
    }

    private void checkWritable()
    {
        if (sendrecv("disk:?write").equals("-"))
            throw new FluxEngineException("cannot write --- disk is write protected");
        if (sendrecv("?safe").equals("+"))
            throw new FluxEngineException("cannot write --- Applesauce 'safe' switch is on");
        if (sendrecv("?vers").compareTo("0300") < 0)
            throw new FluxEngineException("cannot write --- need Applesauce firmware 2.0 or above");
    }

    @Override
    public void write(int side, Bytes fldata, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Applesauce");
        checkWritable();

        connect();
        doCommand(String.format("head:side%d", side));
        doCommand("sync:on");
        doCommand("disk:wipe");
        doCommand("data:clear");
        doCommand("disk:wclear");

        Bytes asdata = fluxEngineToApplesauceWriteData(fldata);
        doCommand(String.format("data:>%d", asdata.size()));
        serial.writeBytes(asdata);
        checkCommandResult(serial.readLine());
        doCommand("disk:wcmd0,0");
        doCommand("disk:write");
    }

    @Override
    public void erase(int side, double hardSectorThresholdNs)
    {
        if (hardSectorThresholdNs != 0.0)
            throw new FluxEngineException(
                    "hard sectors are currently unsupported on the " + "Applesauce");
        checkWritable();

        connect();
        doCommand(String.format("disk:side%d", side));
        doCommand("disk:wipe");
    }

    @Override
    public void setDrive(int drive, boolean highDensity, int indexMode)
    {
        if (drive != 0)
            throw new FluxEngineException("the Applesauce only supports drive 0");

        connect();
        doCommand(String.format("dpc:density%s", highDensity ? "+" : "-"));
    }

    @Override
    public VoltageMeasurements measureVoltages()
    {
        throw new FluxEngineException("unsupported operation on the Applesauce");
    }

    @Override
    public void close()
    {
        try
        {
            sendrecv("disconnect");
        } finally
        {
            serial.close();
        }
    }
}
