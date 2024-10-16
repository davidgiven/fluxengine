#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/fluxmapreader.h"
#include "lib/decoders/fluxdecoder.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "protocol.h"
#include "lib/decoders/rawbits.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"

static FlagGroup flags;

static StringFlag sourceFlux({"--source", "-s"},
    "'drive:' flux source to use",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSource(value);
    });

static IntFlag trackFlag({"--cylinder", "-c"}, "Track to read.", 0);

static IntFlag headFlag({"--head", "-h"}, "Head to read.", 0);

static SettableFlag dumpFluxFlag(
    {"--dump-flux", "-F"}, "Dump raw magnetic disk flux.");

static SettableFlag dumpBitstreamFlag(
    {"--dump-bitstream", "-B"}, "Dump aligned bitstream.");

static IntFlag dumpRawFlag(
    {"--dump-raw", "-R"}, "Dump raw binary with offset.", 0);

static SettableFlag dumpMfmFm(
    {"--mfmfm"}, "When dumping raw binary, do MFM/FM decoding first.");

static SettableFlag dumpBytecodesFlag(
    {"--dump-bytecodes", "-H"}, "Dump the raw FluxEngine bytecodes.");

static IntFlag fluxmapResolutionFlag({"--fluxmap-resolution"},
    "Resolution of flux visualisation (nanoseconds). 0 to autoscale",
    0);

static DoubleFlag seekFlag({"--seek", "-S"},
    "Seek this many milliseconds into the track before displaying it.",
    0.0);

static DoubleFlag manualClockRate({"--manual-clock-rate-us", "-u"},
    "If not zero, force this clock rate; if zero, try to autodetect it.",
    0.0);

static DoubleFlag noiseFloorFactor({"--noise-floor-factor"},
    "Clock detection noise floor (min + (max-min)*factor).",
    0.01);

static DoubleFlag signalLevelFactor({"--signal-level-factor"},
    "Clock detection signal level (min + (max-min)*factor).",
    0.05);

void setDecoderManualClockRate(double clockrate_us)
{
    manualClockRate.setDefaultValue(clockrate_us);
}

static const std::string BLOCK_ELEMENTS[] = {
    " ", "▏", "▎", "▍", "▌", "▋", "▊", "▉", "█"};

/*
 * Tries to guess the clock by finding the smallest common interval.
 * Returns nanoseconds.
 */
static nanoseconds_t guessClock(const Fluxmap& fluxmap)
{
    if (manualClockRate != 0.0)
        return manualClockRate * 1000.0;

    auto data = FluxmapReader(fluxmap).guessClock(
        noiseFloorFactor.get(), signalLevelFactor.get());

    std::cout << "\nClock detection histogram:" << std::endl;

    uint32_t max =
        *std::max_element(std::begin(data.buckets), std::end(data.buckets));

    bool skipping = true;
    for (int i = 0; i < 256; i++)
    {
        nanoseconds_t value = data.buckets[i];
        if (value < data.noiseFloor / 2)
        {
            if (!skipping)
                std::cout << "..." << std::endl;
            skipping = true;
        }
        else
        {
            skipping = false;

            int bar = 320 * value / max;
            int fullblocks = bar / 8;

            std::string s;
            for (int j = 0; j < fullblocks; j++)
                s += BLOCK_ELEMENTS[8];
            s += BLOCK_ELEMENTS[bar & 7];

            std::cout << fmt::format(
                "{: 3} {:.2f} {:6} {}", i, (double)i * US_PER_TICK, value, s);
            std::cout << std::endl;
        }
    }

    std::cout << fmt::format("Noise floor:  {}\n", data.noiseFloor);
    std::cout << fmt::format("Signal level: {}\n", data.signalLevel);
    std::cout << fmt::format(
        "Peak start:   {:.2f} us\n", data.peakStart / 1000.0);
    std::cout << fmt::format(
        "Peak end:     {:.2f} us\n", data.peakEnd / 1000.0);
    std::cout << fmt::format("Median:       {:.2f} us\n", data.median / 1000.0);

    /*
     * Okay, the median should now be a good candidate for the (or a) clock.
     * How this maps onto the actual clock rate depends on the encoding.
     */

    return data.median;
}

int mainInspect(int argc, const char* argv[])
{
    globalConfig().overrides()->mutable_flux_source()->set_type(FLUXTYPE_DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    auto fluxSource = FluxSource::create(globalConfig());
    const auto fluxmap = fluxSource->readFlux(trackFlag, headFlag)->next();

    std::cout << fmt::format("0x{:x} bytes of data in {:.3f}ms\n",
        fluxmap->bytes(),
        fluxmap->duration() / 1e6);
    std::cout << fmt::format("Required USB bandwidth: {}kB/s\n",
        (int)(fluxmap->bytes() / 1024.0 / (fluxmap->duration() / 1e9)));

    nanoseconds_t clockPeriod = guessClock(*fluxmap);
    std::cout << fmt::format(
                     "{:.2f} us clock detected.", (double)clockPeriod / 1000.0)
              << std::flush;

    FluxmapReader fmr(*fluxmap);
    fmr.seek(seekFlag * 1000000.0);

    if (dumpFluxFlag)
    {
        std::cout << "\n\nMagnetic flux follows (times in us):" << std::endl;

        int resolution = fluxmapResolutionFlag;
        if (resolution == 0)
            resolution = clockPeriod / 4;

        nanoseconds_t nextclock = clockPeriod;

        nanoseconds_t now = fmr.tell().ns();
        int ticks = now / NS_PER_TICK;

        std::cout << fmt::format("{: 10.3f}:-", ticks * US_PER_TICK);
        nanoseconds_t lasttransition = 0;
        while (!fmr.eof())
        {
            unsigned thisTicks;
            fmr.findEvent(F_BIT_PULSE, thisTicks);
            ticks += thisTicks;

            nanoseconds_t transition = ticks * NS_PER_TICK;
            nanoseconds_t next;

            bool clocked = false;

            bool bannered = false;
            auto banner = [&]()
            {
                std::cout << fmt::format("\n{: 10.3f}:{}",
                    (double)next / 1000.0,
                    clocked ? '-' : ' ');
                bannered = true;
            };

            for (;;)
            {
                next = now + resolution;
                clocked = now >= nextclock;
                if (clocked)
                    nextclock += clockPeriod;
                if (next >= transition)
                    break;
                banner();
                now = next;
            }

            nanoseconds_t length = transition - lasttransition;
            if (!bannered)
                banner();
            std::cout << fmt::format(
                "==== {:06x} {: 10.3f} +{:.3f} = {:.1f} clocks",
                fmr.tell().bytes,
                (double)transition / 1000.0,
                (double)length / 1000.0,
                (double)length / clockPeriod);
            bannered = false;
            lasttransition = transition;
        }
    }

    if (dumpBitstreamFlag)
    {
        std::cout << fmt::format(
            "\n\nAligned bitstream from {:.3f}ms follows:\n",
            fmr.tell().ns() / 1000000.0);

        FluxDecoder decoder(&fmr, clockPeriod, globalConfig()->decoder());
        while (!fmr.eof())
        {
            std::cout << fmt::format("{:06x} {: 10.3f} : ",
                fmr.tell().bytes,
                fmr.tell().ns() / 1000000.0);
            for (unsigned i = 0; i < 50; i++)
            {
                if (fmr.eof())
                    break;
                bool b = decoder.readBit();
                std::cout << (b ? 'X' : '-');
            }

            std::cout << std::endl;
        }
    }

    if (dumpRawFlag.isSet())
    {
        std::cout << fmt::format(
            "\n\nRaw binary with offset {} from {:.3f}ms follows:\n",
            dumpRawFlag.get(),
            fmr.tell().ns() / 1000000.0);

        FluxDecoder decoder(&fmr, clockPeriod, globalConfig()->decoder());
        for (int i = 0; i < dumpRawFlag; i++)
            decoder.readBit();

        while (!fmr.eof())
        {
            std::cout << fmt::format("{:06x} {: 10.3f} : ",
                fmr.tell().bytes,
                fmr.tell().ns() / 1000000.0);

            Bytes bytes;
            if (dumpMfmFm)
                bytes = decodeFmMfm(decoder.readBits(32 * 8));
            else
                bytes = toBytes(decoder.readBits(16 * 8));

            ByteReader br(bytes);

            for (unsigned i = 0; i < 16; i++)
            {
                if (br.eof())
                    break;
                std::cout << fmt::format("{:02x} ", br.read_8());
            }

            std::cout << std::endl;
        }
    }
    std::cout << std::endl;

    if (dumpBytecodesFlag)
    {
        std::cout << "Raw FluxEngine bytecodes follow:" << std::endl;

        const auto& bytes = fluxmap->rawBytes();
        hexdump(std::cout, bytes);
    }

    return 0;
}
