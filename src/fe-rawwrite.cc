#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/algorithms/readerwriter.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "fluxengine.h"
#include <fstream>
#include <ctype.h>

static FlagGroup flags;

static StringFlag sourceFlux({"--source", "-s"},
    "source flux file to read from",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSource(value);
    });

static StringFlag destFlux({"--dest", "-d"},
    "flux destination to write to",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSink(value);
    });

static StringFlag destTracks({"--cylinders", "-c"},
    "tracks to write to",
    "",
    [](const auto& value)
    {
        setRange(globalConfig().overrides()->mutable_tracks(), value);
    });

static StringFlag destHeads({"--heads", "-h"},
    "heads to write to",
    "",
    [](const auto& value)
    {
        setRange(globalConfig().overrides()->mutable_heads(), value);
    });

static ActionFlag eraseFlag({"--erase"},
    "erases the destination",
    []()
    {
        globalConfig().overrides()->mutable_flux_source()->set_type(
            FLUXTYPE_ERASE);
    });

int mainRawWrite(int argc, const char* argv[])
{
    setRange(globalConfig().overrides()->mutable_tracks(), "0-79");
    setRange(globalConfig().overrides()->mutable_heads(), "0-1");

    if (argc == 1)
        showProfiles("rawwrite", formats);
    globalConfig().overrides()->mutable_flux_sink()->set_type(FLUXTYPE_DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    if (globalConfig()->flux_source().type() == FLUXTYPE_DRIVE)
        error("you can't use rawwrite to read from hardware");

    auto fluxSource = FluxSource::create(globalConfig());
    auto fluxSink = FluxSink::create(globalConfig());

    writeRawDiskCommand(*fluxSource, *fluxSink);
    return 0;
}
