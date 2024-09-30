#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/readerwriter.h"
#include "lib/fluxmap.h"
#include "lib/decoders/decoders.h"
#include "lib/sector.h"
#include "lib/proto.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
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

static StringFlag destImage({"-o", "--output"},
    "destination image to write",
    "",
    [](const auto& value)
    {
        globalConfig().setImageWriter(value);
    });

static StringFlag copyFluxTo({"--copy-flux-to"},
    "while reading, copy the read flux to this file",
    "",
    [](const auto& value)
    {
        globalConfig().setCopyFluxTo(value);
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

int mainRead(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("read", formats);
    globalConfig().set("flux_source.type", "FLUXTYPE_DRIVE");
    flags.parseFlagsWithConfigFiles(argc, argv, formats);
    auto usb = USB::create();

    if (globalConfig()->decoder().copy_flux_to().type() == FLUXTYPE_DRIVE)
        error("you cannot copy flux to a hardware device");

    auto& fluxSource = globalConfig().getFluxSource();
    auto& decoder = globalConfig().getDecoder();
    auto writer = globalConfig().getImageWriter();

    readDiskCommand(*fluxSource, *decoder, *writer);

    return 0;
}
