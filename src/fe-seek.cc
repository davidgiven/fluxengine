#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.cc"
#include "proto.h"
#include "protocol.h"

static FlagGroup flags;

static StringFlag sourceFlux(
	{ "-s", "--source" },
	"'drive:' flux source to use",
	"",
	[](const auto& value)
	{
		FluxSource::updateConfigForFilename(config.mutable_flux_source(), value);
	});

static IntFlag cylinder(
	{ "--cylinder", "-c" },
	"cylinder to seek to",
	0);

extern const std::map<std::string, std::string> readables;

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

	if (!config.flux_source().has_drive())
		Error() << "this only makes sense with a real disk drive";

    usbSetDrive(config.flux_source().drive().drive(), false, config.flux_source().drive().index_mode());
	usbSeek(cylinder);
    return 0;
}
