#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "writer.h"
#include "sector.h"
#include "track.h"
#include "proto.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include <fstream>
#include <ctype.h>

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "--source", "-s" },
	"source flux file to read from",
	"",
	[](const auto& value)
	{
		FluxSource::updateConfigForFilename(config.mutable_input()->mutable_flux(), value);
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

static ActionFlag eraseFlag(
	{ "--erase" },
	"erases the destination",
	[]()
	{
		config.mutable_input()->mutable_flux()->mutable_erase();
	});

int mainRawWrite(int argc, const char* argv[])
{
	config.mutable_output()->mutable_flux()->mutable_drive()->set_drive(0);
	setRange(config.mutable_cylinders(), "0-79");
	setRange(config.mutable_heads(), "0-1");

	if (argc == 1)
		showProfiles("rawwrite", writables);
    flags.parseFlagsWithConfigFiles(argc, argv, writables);

	if (!config.input().has_flux() || !config.output().has_flux())
		Error() << "incomplete config (did you remember to specify the format?)";
	if (config.input().flux().has_drive())
		Error() << "you can't use rawwrite to read from hardware";
	if (config.input().flux().source_case() == FluxSourceProto::SOURCE_NOT_SET)
		Error() << "no source flux specified";

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.input().flux()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.output().flux()));

	writeRawDiskCommand(*fluxSource, *fluxSink);
    return 0;
}

