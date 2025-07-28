#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/data/flux.h"
#include "lib/external/fl2.h"
#include "lib/external/fl2.pb.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;
static std::string filename;

int mainFluxfileLs(int argc, const char* argv[])
{
    const auto filenames = flags.parseFlagsWithFilenames(argc, argv);
    if (filenames.size() != 1)
        error("you must specify exactly one filename");

    const auto& filename = *filenames.begin();
    fmt::print("Contents of {}:\n", filename);
    FluxFileProto f = loadFl2File(filename);

    fmt::print("version: {}\n", getProtoByString(&f, "version"));
    fmt::print("rotational_period_ms: {}\n",
        getProtoByString(&f, "rotational_period_ms"));
    fmt::print("drive_type: {}\n", getProtoByString(&f, "drive_type"));
    fmt::print("format_type: {}\n", getProtoByString(&f, "format_type"));
    for (const auto& track : f.track())
    {
        for (int i = 0; i < track.flux().size(); i++)
        {
            const auto& flux = track.flux().at(i);
            Fluxmap fluxmap(flux);

            fmt::print("track.t{}_h{}.flux{}: {:.3f} ms, {} bytes\n",
                track.track(),
                track.head(),
                i,
                fluxmap.duration() / 1000000,
                fluxmap.bytes());
        }
    }

    return 0;
}
