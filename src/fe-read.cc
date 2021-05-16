#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "macintosh/macintosh.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "proto.h"
#include "dataspec.h"
#include "fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "-s", "--source" },
	"flux file to read from",
	"",
	[](const auto& value)
	{
		config.mutable_input()->mutable_flux()->set_fluxfile(value);
	});

static IntFlag sourceDrive(
	{ "-D", "--drive" },
	"drive to read from",
	0,
	[](const auto& value)
	{
		config.mutable_input()->mutable_flux()->mutable_drive()->set_drive(value);
	});

static StringFlag destImage(
	{ "-o", "--output" },
	"destination image to write",
	"",
	[](const auto& value)
	{
		ImageWriter::updateConfigForFilename(value);
	});

static StringFlag srcCylinders(
	{ "--cylinders", "-c" },
	"cylinders to read from",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_cylinders(), value);
	});

static StringFlag srcHeads(
	{ "--heads", "-h" },
	"heads to read from",
	"",
	[](const auto& value)
	{
		setRange(config.mutable_heads(), value);
	});

extern const std::map<std::string, std::string> readables;

int mainRead(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, readables);

	if (!config.input().has_flux() || !config.output().has_image())
		Error() << "incomplete config (did you remember to specify the format?)";

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.input().flux()));
	std::unique_ptr<AbstractDecoder> decoder(AbstractDecoder::create(config.decoder()));
	std::unique_ptr<ImageWriter> writer(ImageWriter::create(config.output().image()));

	readDiskCommand(*fluxSource, *decoder, *writer);

    return 0;
}

