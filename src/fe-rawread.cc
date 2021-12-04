#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "macintosh/macintosh.h"
#include "sector.h"
#include "proto.h"
#include "fluxsink/fluxsink.h"
#include "fluxsource/fluxsource.h"
#include "arch/brother/brother.h"
#include "arch/ibm/ibm.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "-s", "--source" },
	"flux file to read from",
	"",
	[](const auto& value)
	{
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

static StringFlag destFlux(
	{ "-d", "--dest" },
	"destination flux file to write to",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_flux_sink(), value);
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

int mainRawRead(int argc, const char* argv[])
{
	setRange(config.mutable_cylinders(), "0-79");
	setRange(config.mutable_heads(), "0-1");

	if (argc == 1)
		showProfiles("rawread", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	if (config.flux_sink().has_drive())
		Error() << "you can't use rawread to write to hardware";

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.flux_source()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.flux_sink()));

	rawReadDiskCommand(*fluxSource, *fluxSink);

    return 0;
}

