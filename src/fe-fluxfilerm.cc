#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/data/locations.h"
#include "lib/data/decoded.h"
#include "lib/external/fl2.h"
#include "lib/external/fl2.pb.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;

static StringFlag fluxFilename({"-f", "--fluxfile"}, "flux file to show");
static StringFlag tracksFlag({"-t", "--tracks"}, "tracks to remove");

int mainFluxfileRm(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    if (!fluxFilename.isSet())
        error("you must specify a filename with -f");

    fmt::print("{}:\n", fluxFilename.get());
    FluxFileProto f = loadFl2File(fluxFilename.get());

    bool changed = false;
    for (const auto& location : parseCylinderHeadsString(tracksFlag))
    {
        auto* repeatedFlux = f.mutable_track();

        bool found = false;
        for (int i = 0; i < repeatedFlux->size(); i++)
        {
            const auto& trackFlux = repeatedFlux->Get(i);
            if ((trackFlux.track() == location.cylinder) &&
                (trackFlux.head() == location.head))
            {
                fmt::print(
                    "  removing c{}h{}\n", location.cylinder, location.head);
                repeatedFlux->DeleteSubrange(i, 1);
                found = changed = true;
                i--;
            }
        }

        if (!found)
            fmt::print("  location c{}h{} not found\n",
                location.cylinder,
                location.head);
    }

    if (changed)
    {
        fmt::print("writing back file\n");
        saveFl2File(fluxFilename.get(), f);
    }
    else
        fmt::print("file not modified\n");

    return 0;
}
