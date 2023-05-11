#include "globals.h"
#include "flags.h"
#include "readerwriter.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
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
        setRange(globalConfig()->mutable_tracks(), value);
    });

static StringFlag destHeads({"--heads", "-h"},
    "heads to write to",
    "",
    [](const auto& value)
    {
        setRange(globalConfig()->mutable_heads(), value);
    });

static ActionFlag eraseFlag({"--erase"},
    "erases the destination",
    []()
    {
        globalConfig()->mutable_flux_source()->set_type(FluxSourceProto::ERASE);
    });

int mainRawWrite(int argc, const char* argv[])
{
    setRange(globalConfig()->mutable_tracks(), "0-79");
    setRange(globalConfig()->mutable_heads(), "0-1");

    if (argc == 1)
        showProfiles("rawwrite", formats);
    globalConfig()->mutable_flux_sink()->set_type(FluxSinkProto::DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    if (globalConfig()->flux_source().type() == FluxSourceProto::DRIVE)
        error("you can't use rawwrite to read from hardware");

    std::unique_ptr<FluxSource> fluxSource(
        FluxSource::create(globalConfig()->flux_source()));
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    writeRawDiskCommand(*fluxSource, *fluxSink);
    return 0;
}
