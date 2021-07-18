#include "globals.h"
#include "flags.h"
#include "writer.h"
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
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

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

static StringFlag destCylinders(
	{ "--cylinders", "-c" },
	"cylinders to write to",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_cylinders(), value);
	});

static StringFlag destHeads(
	{ "--heads", "-h" },
	"heads to write to",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_heads(), value);
	});

int mainWrite(int argc, const char* argv[])
{
	if (argc == 1)
		showProfiles("write", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.image_reader()));
	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.flux_sink()));

	std::unique_ptr<AbstractDecoder> decoder;
	if (config.has_decoder())
		decoder = AbstractDecoder::create(config.decoder());

	std::unique_ptr<FluxSource> fluxSource;
	if (config.has_flux_source())
		fluxSource = FluxSource::create(config.flux_source());

	writeDiskCommand(*reader, *encoder, *fluxSink, decoder.get(), fluxSource.get());

    return 0;
}

