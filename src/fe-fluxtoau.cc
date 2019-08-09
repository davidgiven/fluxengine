#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sql.h"
#include "bytes.h"
#include "protocol.h"
#include "dataspec.h"
#include "fluxsource/fluxsource.h"
#include "decoders/fluxmapreader.h"
#include "fmt/format.h"
#include <fstream>

static FlagGroup flags { &hardwareFluxSourceFlags };

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":d=0:t=0:s=0");

static StringFlag output(
    { "--output", "-o" },
    "output AU file to write",
    "output.au");

static SettableFlag highDensityFlag(
    { "--high-density", "--hd" },
    "set the drive to high density mode");

static SettableFlag withIndex(
    { "--with-index" },
    "place index markers in the right hand channel");

int mainConvertFluxToAu(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    FluxSpec spec(source);
    const auto& locations = spec.locations;
    if (locations.size() != 1)
        Error() << "the source dataspec must contain exactly one track (two sides count as two tracks)";
    const auto& location = *(locations.begin());

    std::cerr << "Reading source flux...\n";
    setHardwareFluxSourceDensity(highDensityFlag);
    std::shared_ptr<FluxSource> fluxsource = FluxSource::create(spec);
    const auto& fluxmap = fluxsource->readFlux(location.track, location.side);
    unsigned totalTicks = fluxmap->ticks() + 2;
    unsigned channels = withIndex ? 2 : 1;

    std::cerr << "Writing output file...\n";
    std::ofstream of(output, std::ios::out | std::ios::binary);
    if (!of.is_open())
        Error() << "cannot open output file";

    /* Write header */

    {
        Bytes header;
        header.resize(24);
        ByteWriter bw(header);

        bw.write_be32(0x2e736e64);
        bw.write_be32(24);
        bw.write_be32(totalTicks * channels);
        bw.write_be32(2); /* 8-bit PCM */
        bw.write_be32(TICK_FREQUENCY);
        bw.write_be32(channels); /* channels */

        of.write((const char*) header.cbegin(), header.size());
    }

    /* Write data */

    {
        Bytes data;
        data.resize(totalTicks * channels);
        memset(data.begin(), 0x80, data.size());

        FluxmapReader fmr(*fluxmap);
        unsigned timestamp = 0;
        while (!fmr.eof())
        {
            unsigned ticks;
            int op = fmr.readOpcode(ticks);
            if (op == -1)
                break;
            timestamp += ticks;

            if (op == F_OP_PULSE)
                data[timestamp*channels + 0] = 0x7f;
            if (withIndex && (op == F_OP_INDEX))
                data[timestamp*channels + 1] = 0x7f;
        }

        of.write((const char*) data.cbegin(), data.size());
    }

    std::cerr << "Done. Warning: do not play this file, or you will break your speakers"
	         " and/or ears!\n";
    return 0;
}
