#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "proto.h"
#include "protocol.h"

static FlagGroup flags;

static StringFlag sourceFlux({"-s", "--source"},
    "'drive:' flux source to use",
    "",
    [](const auto& value)
    {
        FluxSource::updateConfigForFilename(
            globalConfig()->mutable_flux_source(), value);
    });

static IntFlag track({"--cylinder", "-c"}, "track to seek to", 0);

extern const std::map<std::string, std::string> readables;

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    if (globalConfig()->flux_source().type() != FluxSourceProto::DRIVE)
        error("this only makes sense with a real disk drive");

    usbSetDrive(globalConfig()->drive().drive(),
        false,
        globalConfig()->drive().index_mode());
    usbSeek(track);
    return 0;
}
