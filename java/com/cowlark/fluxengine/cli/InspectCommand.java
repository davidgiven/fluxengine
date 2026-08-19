package com.cowlark.fluxengine.cli;

import static com.cowlark.fluxengine.wiring.FluxEngine.F_BIT_PULSE;
import static com.cowlark.fluxengine.wiring.FluxEngine.NS_PER_TICK;
import static com.cowlark.fluxengine.wiring.FluxEngine.US_PER_TICK;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.core.flags.DoubleFlag;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.IntFlag;
import com.cowlark.fluxengine.core.flags.SettableFlag;
import com.cowlark.fluxengine.core.flags.StringFlag;
import com.cowlark.fluxengine.core.flags.ValueFlag;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.decoders.FluxDecoder;
import com.cowlark.fluxengine.external.FmMfm;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;

/**
 * Low-level analysis and inspection of a disk, modelled after
 * src/fe-inspect.cc.
 */
public class InspectCommand implements Command
{
    private static final String[] BLOCK_ELEMENTS = {" ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};

    private FlagGroup flags = new FlagGroup();
    private ValueFlag<String> sourceFluxFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--source")
            .setName("-s")
            .setHelpText("'drive:' flux source to use")
            .build();
    private ValueFlag<String> destTracksFlag = StringFlag.builder()
            .setGroup(flags)
            .setName("--tracks")
            .setName("-t")
            .setHelpText("tracks to write to")
            .setDefaultValue("c0h0")
            .build();
    private SettableFlag dumpFluxFlag = SettableFlag.builder()
            .setGroup(flags)
            .setName("--dump-flux")
            .setName("-F")
            .setHelpText("Dump raw magnetic disk flux.")
            .build();
    private SettableFlag dumpBitstreamFlag = SettableFlag.builder()
            .setGroup(flags)
            .setName("--dump-bitstream")
            .setName("-B")
            .setHelpText("Dump aligned bitstream.")
            .build();
    private ValueFlag<Integer> dumpRawFlag = IntFlag.builder()
            .setGroup(flags)
            .setName("--dump-raw")
            .setName("-R")
            .setHelpText("Dump raw binary with offset.")
            .build();
    private SettableFlag dumpMfmFmFlag = SettableFlag.builder()
            .setGroup(flags)
            .setName("--mfmfm")
            .setHelpText("When dumping raw binary, do MFM/FM decoding first.")
            .build();
    private SettableFlag dumpBytecodesFlag = SettableFlag.builder()
            .setGroup(flags)
            .setName("--dump-bytecodes")
            .setName("-H")
            .setHelpText("Dump the raw FluxEngine bytecodes.")
            .build();
    private ValueFlag<Integer> fluxmapResolutionFlag = IntFlag.builder()
            .setGroup(flags)
            .setName("--fluxmap-resolution")
            .setHelpText("Resolution of flux visualisation (nanoseconds). 0 to autoscale")
            .build();
    private ValueFlag<Double> seekFlag = DoubleFlag.builder()
            .setGroup(flags)
            .setName("--seek")
            .setName("-S")
            .setHelpText("Seek this many milliseconds into the track before displaying it.")
            .build();
    private ValueFlag<Double> manualClockRateFlag = DoubleFlag.builder()
            .setGroup(flags)
            .setName("--manual-clock-rate-us")
            .setName("-u")
            .setHelpText("If not zero, force this clock rate; if zero, try to autodetect it.")
            .setDefaultValue(0.0)
            .build();
    private ValueFlag<Double> noiseFloorFactorFlag = DoubleFlag.builder()
            .setGroup(flags)
            .setName("--noise-floor-factor")
            .setHelpText("Clock detection noise floor (min + (max-min)*factor).")
            .setDefaultValue(0.01)
            .build();
    private ValueFlag<Double> signalLevelFactorFlag = DoubleFlag.builder()
            .setGroup(flags)
            .setName("--signal-level-factor")
            .setHelpText("Clock detection signal level (min + (max-min)*factor).")
            .setDefaultValue(0.05)
            .build();

    @Override
    public String getHelp()
    {
        return "Low-level analysis and inspection of a disk.";
    }

    private double guessClock(Fluxmap fluxmap, FluxmapReader fmr)
    {
        double manualClockRate = manualClockRateFlag.get();
        if (manualClockRate != 0.0)
            return manualClockRate * 1000.0;

        FluxmapReader.ClockData data =
                fmr.guessClock(noiseFloorFactorFlag.get(), signalLevelFactorFlag.get());

        System.out.println("\nClock detection histogram:");

        int max = Integer.MIN_VALUE;
        for (int b : data.buckets)
            max = Math.max(max, b);
        if (max == 0)
            max = 1;

        boolean skipping = true;
        for (int i = 0; i < 256; i++)
        {
            int value = data.buckets[i];
            if (value < data.noiseFloor / 2)
            {
                if (!skipping)
                    System.out.println("...");
                skipping = true;
            } else
            {
                skipping = false;

                int bar = 320 * value / max;
                int fullblocks = bar / 8;

                StringBuilder s = new StringBuilder();
                for (int j = 0; j < fullblocks; j++)
                    s.append(BLOCK_ELEMENTS[8]);
                s.append(BLOCK_ELEMENTS[bar & 7]);

                System.out.printf("%3d %.2f %7d %s%n", i, i * US_PER_TICK, value, s);
            }
        }

        System.out.printf("Noise floor:  %d%n", data.noiseFloor);
        System.out.printf("Signal level: %d%n", data.signalLevel);
        System.out.printf("Peak start:   %.2f us%n", data.peakStartTicks * US_PER_TICK);
        System.out.printf("Peak end:     %.2f us%n", data.peakEndTicks * US_PER_TICK);
        System.out.printf("Median:       %.2f us%n", data.medianTicks * US_PER_TICK);

        return data.medianTicks * NS_PER_TICK;
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigBuilder builder = new ConfigBuilder().fromFlags(args, flags);
        if (sourceFluxFlag.isSet())
            builder.withFluxSource(sourceFluxFlag.get());
        ConfigProto config = builder.build();

        try (UsbFactory usbFactory = new UsbFactory(config))
        {
            FluxSource fluxSource = FluxSource.create(config, () -> usbFactory);
            ImmutableList<CylinderHead> tracks =
                    Locations.parseCylinderHeadsString(destTracksFlag.get());
            if (tracks.size() != 1)
                throw new FluxEngineException("you must specify exactly one track");
            CylinderHead ch = tracks.get(0);
            FluxSourceIterator iterator = fluxSource.readFlux(FluxReadParameters.builder()
                    .setCylinder(ch.cylinder())
                    .setHead(ch.head())
                    .build());
            Fluxmap fluxmap = iterator.next();

            System.out.printf("0x%x bytes of data in %.3fms%n",
                    fluxmap.bytes(),
                    fluxmap.durationNs() / 1e6);
            System.out.printf("Required USB bandwidth: %dkB/s%n",
                    (int) (fluxmap.bytes() / 1024.0 / (fluxmap.durationNs() / 1e9)));

            FluxmapReader fmr = new FluxmapReader(fluxmap, DecoderProto.getDefaultInstance());
            double clockPeriod = guessClock(fluxmap, fmr);
            System.out.printf("%.2f us clock detected.", clockPeriod / 1000.0);
            System.out.flush();

            fmr.seek((long) (seekFlag.get() * 1000000.0 / NS_PER_TICK));

            if (dumpFluxFlag.get())
            {
                System.out.println("\n\nMagnetic flux follows (times in us):");

                int resolution = fluxmapResolutionFlag.get();
                if (resolution == 0)
                    resolution = (int) (clockPeriod / 4);

                double nextclock = clockPeriod;

                double now = fmr.tell().getDurationNs();
                long ticks = (long) (now / NS_PER_TICK);

                System.out.printf("%10.3f:-", ticks * US_PER_TICK);
                double lasttransition = 0;
                while (!fmr.eof())
                {
                    FluxmapReader.EventResult r = fmr.findEvent(F_BIT_PULSE);
                    long thisTicks = r.ticks();
                    ticks += thisTicks;

                    double transition = ticks * NS_PER_TICK;
                    double next;

                    boolean clocked = false;

                    boolean bannered = false;
                    for (; ; )
                    {
                        next = now + resolution;
                        clocked = now >= nextclock;
                        if (clocked)
                            nextclock += clockPeriod;
                        if (next >= transition)
                            break;
                        if (!bannered)
                        {
                            System.out.printf("%n%10.3f:%c", next / 1000.0, clocked ? '-' : ' ');
                            bannered = true;
                        }
                        now = next;
                    }

                    double length = transition - lasttransition;
                    if (!bannered)
                    {
                        System.out.printf("%n%10.3f:%c", next / 1000.0, clocked ? '-' : ' ');
                        bannered = true;
                    }
                    System.out.printf("==== %06x %10.3f +%.3f = %.1f clocks",
                            fmr.tell().bytes(),
                            transition / 1000.0,
                            length / 1000.0,
                            length / clockPeriod);
                    lasttransition = transition;
                }
            }

            if (dumpBitstreamFlag.get())
            {
                System.out.printf("\n\nAligned bitstream from %.3fms follows:%n",
                        fmr.tell().getDurationNs() / 1000000.0);

                FluxDecoder decoder = new FluxDecoder(fmr, clockPeriod, config.getDecoder());
                while (!fmr.eof())
                {
                    System.out.printf("%06x %10.3f : ",
                            fmr.tell().bytes(),
                            fmr.tell().getDurationNs() / 1000000.0);
                    for (int i = 0; i < 50; i++)
                    {
                        if (fmr.eof())
                            break;
                        boolean b = decoder.readBit();
                        System.out.print(b ? 'X' : '-');
                    }

                    System.out.println();
                }
            }

            if (dumpRawFlag.isSet())
            {
                System.out.printf("\n\nRaw binary with offset %d from %.3fms follows:%n",
                        dumpRawFlag.get(),
                        fmr.tell().getDurationNs() / 1000000.0);

                FluxDecoder decoder = new FluxDecoder(fmr, clockPeriod, config.getDecoder());
                for (int i = 0; i < dumpRawFlag.get(); i++)
                    decoder.readBit();

                while (!fmr.eof())
                {
                    System.out.printf("%06x %10.3f : ",
                            fmr.tell().bytes(),
                            fmr.tell().getDurationNs() / 1000000.0);

                    Bytes bytes;
                    if (dumpMfmFmFlag.get())
                        bytes = FmMfm.decodeFmMfm(decoder.readBits(32 * 8));
                    else
                        bytes = decoder.readBits(16 * 8).toBytes();

                    for (int i = 0; i < 16; i++)
                    {
                        if (i >= bytes.size())
                            break;
                        System.out.printf("%02x ", bytes.getByte(i) & 0xff);
                    }

                    System.out.println();
                }
            }
            System.out.println();

            if (dumpBytecodesFlag.get())
            {
                System.out.println("Raw FluxEngine bytecodes follow:");

                Utils.hexdump(System.out, fluxmap.rawBytes());
            }
        }
    }
}
