#include "globals.h"
#include "flags.h"
#include "writer.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "encoders/encoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "proto.h"
#include "dataspec.h"
#include "fluxsink/fluxsink.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagereader/imagereader.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

static StringFlag sourceImage(
	{ "-i", "--input" },
	"source image to read from",
	"",
	[](const auto& value)
	{
		ImageReader::updateConfigForFilename(value);
	});

static StringFlag destFlux(
	{ "-d", "--dest" },
	"flux file to write to",
	"",
	[](const auto& value)
	{
		config.mutable_output()->mutable_flux()->set_fluxfile(value);
	});

static IntFlag destDrive(
	{ "-D", "--drive" },
	"drive to write to",
	0,
	[](const auto& value)
	{
		config.mutable_output()->mutable_flux()->mutable_drive()->set_drive(value);
	});

extern const std::map<std::string, std::string> writables;

int mainWrite(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, writables);

	if (!config.input().has_image() || !config.output().has_flux())
		Error() << "incomplete config (did you remember to specify the format?)";

	std::unique_ptr<ImageReader> reader(ImageReader::create(config.input().image()));
	std::unique_ptr<AbstractEncoder> encoder(AbstractEncoder::create(config.encoder()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.output().flux()));

	writeDiskCommand(*reader, *encoder, *fluxSink);

    return 0;
}

