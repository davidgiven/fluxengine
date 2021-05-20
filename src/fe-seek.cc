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
		FluxSource::updateConfigForFilename(config.mutable_input()->mutable_flux(), value);
	});

static IntFlag cylinder(
	{ "--cylinder", "-c" },
	"cylinder to seek to",
	0);

extern const std::map<std::string, std::string> readables;

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, readables);

	if (!config.input().flux().has_drive())
		Error() << "this only makes sense with a real disk drive";

    usbSetDrive(config.input().flux().drive().drive(), false, config.input().flux().drive().index_mode());
	usbSeek(cylinder);
    return 0;
}
