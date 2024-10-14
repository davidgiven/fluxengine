#include "lib/core/globals.h"
#include "lib/data/fluxmap.h"
#include "lib/external/fl2.pb.h"
#include <fstream>

static void upgradeFluxFile(FluxFileProto& proto)
{
    if (proto.version() == FluxFileVersion::VERSION_1)
    {
        /* Change a flux datastream with multiple segments separated by F_DESYNC
         * into multiple flux segments. */

        for (auto& track : *proto.mutable_track())
        {
            if (track.flux_size() != 0)
            {
                Fluxmap oldFlux(track.flux(0));

                track.clear_flux();
                for (const auto& flux : oldFlux.split())
                    track.add_flux(flux->rawBytes());
            }
        }

        proto.set_version(FluxFileVersion::VERSION_2);
    }

    if (proto.version() > FluxFileVersion::VERSION_2)
        error(
            "this is a version {} flux file, but this build of the client can "
            "only handle up to version {} --- please upgrade",
            (int)proto.version(),
            (int)FluxFileVersion::VERSION_2);
}

FluxFileProto loadFl2File(const std::string filename)
{
    std::ifstream ifs(filename, std::ios::in | std::ios::binary);
    if (!ifs.is_open())
        error("cannot open input file '{}': {}", filename, strerror(errno));

    char buffer[16];
    ifs.read(buffer, sizeof(buffer));
    if (strncmp(buffer, "SQLite format 3", 16) == 0)
        error(
            "this flux file is too old; please use the upgrade-flux-file tool "
            "to upgrade it");

    FluxFileProto proto;
    ifs.seekg(0);
    if (!proto.ParseFromIstream(&ifs))
        error("unable to read input file '{}'", filename);
    upgradeFluxFile(proto);
    return proto;
}

void saveFl2File(const std::string filename, FluxFileProto& proto)
{
    proto.set_magic(FluxMagic::MAGIC);
    proto.set_version(FluxFileVersion::VERSION_2);

    std::ofstream of(filename, std::ios::out | std::ios::binary);
    if (!proto.SerializeToOstream(&of))
        error("unable to write output file '{}'", filename);
    of.close();
    if (of.fail())
        error("FL2 write I/O error: {}", strerror(errno));
}
