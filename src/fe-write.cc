#include "globals.h"
#include "flags.h"
#include "writer.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "sector.h"
#include "proto.h"
#include "fluxsink/fluxsink.h"
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
		ImageReader::updateConfigForFilename(config.mutable_input()->mutable_image(), value);
	});

static StringFlag destFlux(
	{ "--dest", "-d" },
	"flux destination to write to",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_output()->mutable_flux(), value);
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
		showProfiles("write", writables);
    flags.parseFlagsWithConfigFiles(argc, argv, writables);

	if (!config.input().has_image() || !config.output().has_flux())
		Error() << "incomplete config (did you remember to specify the format?)";

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.input().image()));
	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.output().flux()));

	writeDiskCommand(*reader, *encoder, *fluxSink);

    return 0;
}

