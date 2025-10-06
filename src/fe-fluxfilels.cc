#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/data/decoded.h"
#include "lib/external/fl2.h"
#include "lib/external/fl2.pb.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;

static StringFlag fluxFilename({"-f", "--fluxfile"}, "flux file to show");

int mainFluxfileLs(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    if (!fluxFilename.isSet())
        error("you must specify a filename with -f");

    fmt::print("{}:\n", fluxFilename.get());
    FluxFileProto f = loadFl2File(fluxFilename.get());

    for (auto* s :
        {"version", "rotational_period_ms", "drive_type", "format_type"})
        fmt::print("  {}: {}\n", s, getProtoByString(&f, s));

    for (const auto& trackFlux : f.track())
    {
        fmt::print("  flux for c{}h{}:", trackFlux.track(), trackFlux.head());

        for (const auto& flux : trackFlux.flux())
        {
            Fluxmap fluxmap(flux);
            if (&flux != &trackFlux.flux()[0])
                fmt::print(",");
            fmt::print(" {:0.3f}ms", fluxmap.duration() / 1000000.0);
        }

        fmt::print("\n");
    }

    return 0;
}
