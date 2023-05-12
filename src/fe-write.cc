#include "globals.h"
#include "flags.h"
#include "readerwriter.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "sector.h"
#include "proto.h"
#include "fluxsink/fluxsink.h"
#include "fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagereader/imagereader.h"
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
        setRange(globalConfig()->mutable_tracks(), value);
    });

static StringFlag destHeads({"--heads", "-h"},
    "heads to write to",
    "",
    [](const auto& value)
    {
        setRange(globalConfig()->mutable_heads(), value);
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

    auto& reader = globalConfig().getImageReader();
    std::shared_ptr<Image> image = reader->readMappedImage();

    std::unique_ptr<Encoder> encoder(
        Encoder::create(globalConfig()->encoder()));
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    std::unique_ptr<Decoder> decoder;
    std::shared_ptr<FluxSource> verificationFluxSource;
    if (globalConfig()->has_decoder() && fluxSink->isHardware() && verify)
    {
        decoder = Decoder::create(globalConfig()->decoder());
        verificationFluxSource = globalConfig().getVerificationFluxSource();
    }

    writeDiskCommand(*image,
        *encoder,
        *fluxSink,
        decoder.get(),
        verificationFluxSource.get());

    return 0;
}
