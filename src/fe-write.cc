#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/readerwriter.h"
#include "lib/fluxmap.h"
#include "lib/decoders/decoders.h"
#include "lib/encoders/encoders.h"
#include "lib/sector.h"
#include "lib/proto.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "lib/imagereader/imagereader.h"
#include "lib/usb/usb.h"
#include "fluxengine.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;
static bool verify = true;

static StringFlag sourceImage({"--input", "-i"},
    "source image to read from",
    "",
    [](const auto& value)
    {
        globalConfig().setImageReader(value);
    });

static StringFlag destFlux({"--dest", "-d"},
    "flux destination to write to",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSink(value);
        globalConfig().setVerificationFluxSource(value);
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

static ActionFlag noVerifyFlag({"--no-verify", "-n"},
    "skip verification of write",
    []
    {
        verify = false;
    });

int mainWrite(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("write", formats);
    globalConfig().setFluxSink("drive:0");
    globalConfig().setVerificationFluxSource("drive:0");

    flags.parseFlagsWithConfigFiles(argc, argv, formats);
    auto usb = USB::create();

    auto& reader = globalConfig().getImageReader();
    std::shared_ptr<Image> image = reader->readMappedImage();

    auto encoder = globalConfig().getEncoder();
    auto fluxSink = globalConfig().getFluxSink();

    std::shared_ptr<Decoder> decoder;
    std::shared_ptr<FluxSource> verificationFluxSource;
    if (globalConfig().hasDecoder() && fluxSink->isHardware() && verify)
    {
        decoder = globalConfig().getDecoder();
        verificationFluxSource = globalConfig().getVerificationFluxSource();
    }

    writeDiskCommand(*image,
        *encoder,
        *fluxSink,
        decoder.get(),
        verificationFluxSource.get());

    return 0;
}
