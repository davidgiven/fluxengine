#include "globals.h"
#include "flags.h"
#include "readerwriter.h"
#include "fluxmap.h"
#include "sector.h"
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
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

static StringFlag destFlux(
	{ "--dest", "-d" },
	"flux destination to write to",
	"",
	[](const auto& value)
	{
		FluxSink::updateConfigForFilename(config.mutable_flux_sink(), value);
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

static ActionFlag eraseFlag(
	{ "--erase" },
	"erases the destination",
	[]()
	{
		config.mutable_flux_source()->set_type(FluxSourceProto::ERASE);
	});

int mainRawWrite(int argc, const char* argv[])
{
	setRange(config.mutable_tracks(), "0-79");
	setRange(config.mutable_heads(), "0-1");

	if (argc == 1)
		showProfiles("rawwrite", formats);
	config.mutable_flux_sink()->set_type(FluxSinkProto::DRIVE);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

	if (config.flux_source().type() == FluxSourceProto::DRIVE)
		Error() << "you can't use rawwrite to read from hardware";

	std::unique_ptr<FluxSource> fluxSource(FluxSource::create(config.flux_source()));
	std::unique_ptr<FluxSink> fluxSink(FluxSink::create(config.flux_sink()));

	writeRawDiskCommand(*fluxSource, *fluxSink);
    return 0;
}

