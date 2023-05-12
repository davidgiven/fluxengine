#include "globals.h"
#include "flags.h"
#include "readerwriter.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "macintosh/macintosh.h"
#include "sector.h"
#include "proto.h"
#include "fluxsink/fluxsink.h"
#include "fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagewriter/imagewriter.h"
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

static StringFlag destFlux({"-d", "--dest"},
    "destination flux file to write to",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSink(value);
    });

static StringFlag srcTracks({"--cylinders", "-c"},
    "tracks to read from",
    "",
    [](const auto& value)
    {
        setRange(globalConfig()->mutable_tracks(), value);
    });

static StringFlag srcHeads({"--heads", "-h"},
    "heads to read from",
    "",
    [](const auto& value)
    {
        setRange(globalConfig()->mutable_heads(), value);
    });

int mainRawRead(int argc, const char* argv[])
{
    setRange(globalConfig()->mutable_tracks(), "0-79");
    setRange(globalConfig()->mutable_heads(), "0-1");

    if (argc == 1)
        showProfiles("rawread", formats);
    globalConfig()->mutable_flux_source()->set_type(FluxSourceProto::DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    if (globalConfig()->flux_sink().type() == FluxSinkProto::DRIVE)
        error("you can't use rawread to write to hardware");

    std::shared_ptr<FluxSource> fluxSource = globalConfig().getFluxSource();
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    rawReadDiskCommand(*fluxSource, *fluxSink);

    return 0;
}
