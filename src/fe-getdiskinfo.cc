#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/fluxmap.h"
#include "lib/sector.h"
#include "lib/proto.h"
#include "lib/readerwriter.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "lib/utils.h"
#include "lib/usb/usb.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

int mainGetDiskInfo(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("getdiskinfo", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    auto usb = USB::create();
    auto filesystem = Filesystem::createFilesystemFromConfig();
    auto attributes = filesystem->getMetadata();

    for (const auto& e : attributes)
        fmt::print("{}={}\n", e.first, quote(e.second));

    return 0;
}
