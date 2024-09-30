#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/readerwriter.h"
#include "lib/fluxmap.h"
#include "lib/decoders/decoders.h"
#include "lib/sector.h"
#include "lib/proto.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/usb/usb.h"
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
        setRange(globalConfig().overrides()->mutable_tracks(), value);
    });

static StringFlag srcHeads({"--heads", "-h"},
    "heads to read from",
    "",
    [](const auto& value)
    {
        setRange(globalConfig().overrides()->mutable_heads(), value);
    });

int mainRawRead(int argc, const char* argv[])
{
    setRange(globalConfig().overrides()->mutable_tracks(), "0-79");
    setRange(globalConfig().overrides()->mutable_heads(), "0-1");

    if (argc == 1)
        showProfiles("rawread", formats);
    globalConfig().overrides()->mutable_flux_source()->set_type(FLUXTYPE_DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);
    auto usb = USB::create();

    if (globalConfig()->flux_sink().type() == FLUXTYPE_DRIVE)
        error("you can't use rawread to write to hardware");

    std::shared_ptr<FluxSource> fluxSource = globalConfig().getFluxSource();
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    rawReadDiskCommand(*fluxSource, *fluxSink);

    return 0;
}
