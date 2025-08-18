#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/data/locations.h"
#include "lib/data/flux.h"
#include "lib/external/fl2.h"
#include "lib/external/fl2.pb.h"
#include "src/fluxengine.h"
#include <fstream>

static FlagGroup flags;

static StringFlag inputFilenameFlag({"-i", "--input"}, "input flux file");
static StringFlag outputFilenameFlag(
    {"-o", "--output"}, "output flux file (must exist)");
static StringFlag tracksFlag({"-t", "--tracks"}, "tracks to copy");

static const TrackFluxProto* findTrack(
    const FluxFileProto& f, int cylinder, int head)
{
    for (const auto& trackFlux : f.track())
        if ((trackFlux.track() == cylinder) && (trackFlux.head() == head))
            return &trackFlux;

    return nullptr;
}

static TrackFluxProto* findOrMakeTrack(FluxFileProto& f, int cylinder, int head)
{
    for (auto& trackFlux : *f.mutable_track())
        if ((trackFlux.track() == cylinder) && (trackFlux.head() == head))
            return &trackFlux;

    TrackFluxProto* tf = f.add_track();
    tf->set_track(cylinder);
    tf->set_head(head);
    return tf;
}

int mainFluxfileCp(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);
    if (!inputFilenameFlag.isSet())
        error("you must specify an input filename with -i");
    if (!outputFilenameFlag.isSet())
        error("you must specify an output filename with -o");

    fmt::print(
        "{} -> {}:\n", inputFilenameFlag.get(), outputFilenameFlag.get());
    FluxFileProto inf = loadFl2File(inputFilenameFlag.get());
    FluxFileProto outf = loadFl2File(outputFilenameFlag.get());

    bool changed = false;
    for (const auto& location : parseCylinderHeadsString(tracksFlag))
    {
        const TrackFluxProto* intrack =
            findTrack(inf, location.cylinder, location.head);
        if (!intrack)
        {
            fmt::print("  location c{}h{} not found\n",
                location.cylinder,
                location.head);
            continue;
        }
        TrackFluxProto* outtrack =
            findOrMakeTrack(outf, location.cylinder, location.head);
        fmt::print("  copying c{}h{}\n", location.cylinder, location.head);
        for (const auto& flux : intrack->flux())
            outtrack->add_flux(flux);
        changed = true;
    }

    if (changed)
    {
        fmt::print("writing back output file\n");
        saveFl2File(outputFilenameFlag.get(), outf);
    }
    else
        fmt::print("output file not modified\n");

    return 0;
}
