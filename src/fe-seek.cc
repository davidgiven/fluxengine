#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/usb/usb.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/config/proto.h"
#include "protocol.h"

static FlagGroup flags;

static StringFlag sourceFlux({"-s", "--source"},
    "'drive:' flux source to use",
    "",
    [](const auto& value)
    {
        globalConfig().setFluxSource(value);
    });

static IntFlag track({"--cylinder", "-c"}, "track to seek to", 0);

extern const std::map<std::string, std::string> readables;

int mainSeek(int argc, const char* argv[])
{
    flags.parseFlagsWithConfigFiles(argc, argv, {});

    if (globalConfig()->flux_source().type() != FLUXTYPE_DRIVE)
        error("this only makes sense with a real disk drive");

    usbSetDrive(globalConfig()->drive().drive(),
        false,
        globalConfig()->drive().index_mode());
    usbSeek(track);
    return 0;
}
