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

static FlagGroup flags;

static StringFlag sourceFlux({"-s", "--source"},
    "flux file to read from",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSource(value);
    });

static StringFlag destImage({"-d", "--dest"},
    "flux file to write to",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSink(value);
    });

int mainConvert(int argc, const char* argv[])
{
    flags.parseFlags(argc, argv);

    if ((globalConfig()->flux_sink().type() == FLUXTYPE_DRIVE) ||
        (globalConfig()->flux_source().type() == FLUXTYPE_DRIVE))
        error("you cannot read or write flux to a hardware device");
    if ((globalConfig()->flux_sink().type() == FLUXTYPE_NOT_SET) ||
        (globalConfig()->flux_source().type() == FLUXTYPE_NOT_SET))
        error("you must specify both a source and destination flux filename");

    auto fluxSource = FluxSource::create(globalConfig());
    auto locations = globalConfig()->drive().tracks();
    globalConfig().overrides()->set_tracks(locations);

    auto diskLayout = createDiskLayout(globalConfig());
    auto [minCylinder, maxCylinder, minHead, maxHead] =
        diskLayout->getPhysicalBounds();
    log("CONVERT: seen cylinders {}..{}, heads {}..{}",
        minCylinder,
        maxCylinder,
        minHead,
        maxHead);

    auto fluxSinkFactory = FluxSink::create(globalConfig());
    auto fluxSink = fluxSinkFactory->create();

    for (const auto& physicalLocation : diskLayout->physicalLocations)
    {
        auto fi = fluxSource->readFlux(physicalLocation);
        while (fi->hasNext())
            fluxSink->addFlux(physicalLocation, *fi->next());
    }

    return 0;
}
