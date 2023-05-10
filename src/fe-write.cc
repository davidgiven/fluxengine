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
        ImageReader::updateConfigForFilename(
            globalConfig()->mutable_image_reader(), value);
    });

static StringFlag destFlux({"--dest", "-d"},
    "flux destination to write to",
    "",
    [](const auto& value)
    {
        FluxSink::updateConfigForFilename(
            globalConfig()->mutable_flux_sink(), value);
        FluxSource::updateConfigForFilename(
            globalConfig()->mutable_flux_source(), value);
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
    globalConfig()->mutable_flux_sink()->set_type(FluxSinkProto::DRIVE);
    if (verify)
        globalConfig()->mutable_flux_source()->set_type(FluxSourceProto::DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    std::unique_ptr<ImageReader> reader(
        ImageReader::create(globalConfig()->image_reader()));
    std::shared_ptr<Image> image = reader->readMappedImage();

    std::unique_ptr<Encoder> encoder(
        Encoder::create(globalConfig()->encoder()));
    std::unique_ptr<FluxSink> fluxSink(
        FluxSink::create(globalConfig()->flux_sink()));

    std::unique_ptr<Decoder> decoder;
    if (globalConfig()->has_decoder() && verify)
        decoder = Decoder::create(globalConfig()->decoder());

    std::unique_ptr<FluxSource> fluxSource;
    if (verify &&
        (globalConfig()->flux_source().type() == FluxSourceProto::DRIVE))
        fluxSource = FluxSource::create(globalConfig()->flux_source());

    writeDiskCommand(
        *image, *encoder, *fluxSink, decoder.get(), fluxSource.get());

    return 0;
}
