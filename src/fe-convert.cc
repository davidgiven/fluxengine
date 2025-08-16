#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/config/proto.h"
#include "lib/config/config.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/data/layout.h"
#include "lib/data/fluxmap.h"
#include "lib/core/logger.h"
#include "fluxengine.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags = {};

static void syntax()
{
    error("syntax: fluxengine convert <sourcefile> <destfile>");
}

int mainConvert(int argc, const char* argv[])
{
    auto filenames = flags.parseFlagsWithFilenames(argc, argv);
    if (filenames.size() != 2)
        syntax();

    globalConfig().setFluxSource(filenames[0]);
    globalConfig().setFluxSink(filenames[1]);

    auto fluxSource = FluxSource::create(globalConfig());

#if 0
    std::vector<std::shared_ptr<const TrackInfo>> locations;
    std::map<std::shared_ptr<const TrackInfo>,
        std::vector<std::shared_ptr<const Fluxmap>>>
        data;
    for (int track = 0; track < 255; track++)
        for (int side = 0; side < 2; side++)
        {
            auto ti = std::make_shared<TrackInfo>();
            ti->physicalTrack = track;
            ti->physicalSide = side;

            auto fsi = fluxSource->readFlux(track, side);
            std::vector<std::shared_ptr<const Fluxmap>> fluxes;

            while (fsi->hasNext())
                fluxes.push_back(fsi->next());

            if (!fluxes.empty())
            {
                data[ti] = fluxes;
                locations.push_back(ti);
            }
        }

    auto [minTrack, maxTrack, minSide, maxSide] = Layout::getBounds(locations);
    log("CONVERT: seen tracks {}..{}, sides {}..{}",
        minTrack,
        maxTrack,
        minSide,
        maxSide);

    globalConfig().set("tracks", fmt::format("{}-{}", minTrack, maxTrack));
    globalConfig().set("heads", fmt::format("{}-{}", minSide, maxSide));

    auto fluxSink = FluxSink::create(globalConfig());
#endif

    return 0;
}
