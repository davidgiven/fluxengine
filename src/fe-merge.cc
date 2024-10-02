#include "lib/core/globals.h"
#include "lib/flags.h"
#include "lib/fluxmap.h"
#include "lib/sector.h"
#include "lib/proto.h"
#include "lib/flux.h"
#include "lib/fl2.h"
#include "lib/fl2.pb.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;

static std::vector<std::string> inputFluxFiles;

static StringFlag sourceFlux({"-s", "--source"},
    "flux file to read from (repeatable)",
    "",
    [](const auto& value)
    {
        inputFluxFiles.push_back(value);
    });

static StringFlag destFlux(
    {"-d", "--dest"}, "destination flux file to write to", "");

int mainMerge(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    if (inputFluxFiles.empty())
        error("you must specify at least one input flux file (with -s)");
    if (destFlux.get() == "")
        error("you must specify an output flux file (with -d)");

    std::map<std::pair<int, int>, TrackFluxProto> data;
    for (const auto& s : inputFluxFiles)
    {
        fmt::print("Reading {}...\n", s);
        FluxFileProto f = loadFl2File(s);

        for (auto& trackflux : f.track())
        {
            auto key = std::make_pair(trackflux.track(), trackflux.head());
            auto i = data.find(key);
            if (i == data.end())
                data[key] = trackflux;
            else
            {
                for (auto flux : trackflux.flux())
                    i->second.add_flux(flux);
            }
        }
    }

    FluxFileProto proto;
    for (auto& i : data)
        *proto.add_track() = i.second;

    fmt::print("Writing {}...\n", destFlux.get());
    saveFl2File(destFlux.get(), proto);

    return 0;
}
