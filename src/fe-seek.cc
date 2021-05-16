#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "proto.h"
#include "protocol.h"

static FlagGroup flags;

static IntFlag drive(
	{ "--drive", "-D" },
	"drive to seek",
	0,
	[](const auto& value)
	{
		config.mutable_input()->mutable_flux()->mutable_drive()->set_drive(value);
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
		Error() << "incomplete config (did you remember to specify the format?)";

    usbSetDrive(config.input().flux().drive().drive(), false, config.input().flux().drive().index_mode());
	usbSeek(cylinder);
    return 0;
}
