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
    "output VCD file to write",
    "output.vcd");

static SettableFlag highDensityFlag(
    { "--high-density", "--hd" },
    "set the drive to high density mode");

int mainConvertFluxToVcd(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    const FluxSpec spec(source);
    const auto& locations = spec.locations;
    if (locations.size() != 1)
        Error() << "the source dataspec must contain exactly one track (two sides count as two tracks)";
    const auto& location = *(locations.begin());

    std::cerr << "Reading source flux...\n";
    setHardwareFluxSourceDensity(highDensityFlag);
    std::shared_ptr<FluxSource> fluxsource = FluxSource::create(spec);
    const auto& fluxmap = fluxsource->readFlux(location.track, location.side);

    std::cerr << "Writing destination VCD...\n";
    std::ofstream of(output, std::ios::out);
    if (!of.is_open())
        Error() << "cannot open output file";

    of << "$timescale 1ns $end\n"
       << "$var wire 1 i index $end\n"
       << "$var wire 1 p pulse $end\n"
       << "$upscope $end\n"
       << "$enddefinitions $end\n"
       << "$dumpvars 0i 0p $end\n";

    FluxmapReader fmr(*fluxmap);
    unsigned timestamp = 0;
    unsigned lasttimestamp = 0;
    while (!fmr.eof())
    {
        unsigned ticks;
        int op = fmr.readOpcode(ticks);
        if (op == -1)
            break;

        unsigned newtimestamp = timestamp + ticks;
        if (newtimestamp != lasttimestamp)
        {
            of << fmt::format("\n#{} 0i 0p\n", (uint64_t)((lasttimestamp+1) * NS_PER_TICK));
            timestamp = newtimestamp;
            of << fmt::format("#{} ", (uint64_t)(timestamp * NS_PER_TICK));
        }

        if (op == F_OP_PULSE)
            of << "1p ";
        if (op == F_OP_INDEX)
            of << "1i ";

        lasttimestamp = timestamp;
    }
    of << "\n";

    return 0;
}
