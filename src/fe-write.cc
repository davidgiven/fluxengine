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
#include "fluxengine.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;
static bool verify = true;

static StringFlag sourceImage(
	{ "--input", "-i" },
	"source image to read from",
	"",
	[](const auto& value)
	{
		ImageReader::updateConfigForFilename(config.mutable_image_reader(), value);
	});

static StringFlag destFlux(
	{ "--dest", "-d" },
	"flux destination to write to",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_flux_sink(), value);
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

static StringFlag destTracks(
	{ "--cylinders", "-c" },
	"tracks to write to",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_tracks(), value);
	});

static StringFlag destHeads(
	{ "--heads", "-h" },
	"heads to write to",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_heads(), value);
	});

static ActionFlag noVerifyFlag(
	{ "--no-verify", "-n" },
	"skip verification of write",
	[]{
		verify = false;
	});

int mainWrite(int argc, const char* argv[])
{
	if (argc == 1)
		showProfiles("write", formats);
	config.mutable_flux_sink()->mutable_drive();
	config.mutable_flux_source()->mutable_drive();
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.image_reader()));
	std::shared_ptr<Image> image = reader->readImage();

	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.flux_sink()));

	std::unique_ptr<AbstractDecoder> decoder;
	if (config.has_decoder() && verify)
		decoder = AbstractDecoder::create(config.decoder());

	std::unique_ptr<FluxSource> fluxSource;
	if (config.has_flux_source() && config.flux_source().has_drive())
		fluxSource = FluxSource::create(config.flux_source());

	writeDiskCommand(image, *encoder, *fluxSink, decoder.get(), fluxSource.get());

    return 0;
}

