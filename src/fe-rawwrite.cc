#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/readerwriter.h"
#include "lib/fluxmap.h"
#include "lib/sector.h"
#include "lib/proto.h"
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
    globalConfig().overrides()->mutable_flux_sink()->set_type(
        FLUXTYPE_DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    if (globalConfig()->flux_source().type() == FLUXTYPE_DRIVE)
        error("you can't use rawwrite to read from hardware");

    auto& fluxSource = globalConfig().getFluxSource();
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    writeRawDiskCommand(*fluxSource, *fluxSink);
    return 0;
}
