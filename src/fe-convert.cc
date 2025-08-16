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
    auto locations = globalConfig()->drive().tracks();
    globalConfig().overrides()->set_tracks(locations);

    auto physicalLocations = Layout::computePhysicalLocations();
    auto [minCylinder, maxCylinder, minHead, maxHead] =
        Layout::getBounds(physicalLocations);
    log("CONVERT: seen cylinders {}..{}, heads {}..{}",
        minCylinder,
        maxCylinder,
        minHead,
        maxHead);

    auto fluxSink = FluxSink::create(globalConfig());

    for (const auto& physicalLocation : physicalLocations)
    {
        auto fi = fluxSource->readFlux(physicalLocation);
        while (fi->hasNext())
            fluxSink->writeFlux(physicalLocation, *fi->next());
    }

    return 0;
}
