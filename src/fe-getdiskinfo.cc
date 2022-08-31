#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
#include "readerwriter.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/decoders/decoders.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "lib/utils.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

int mainGetDiskInfo(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("getdiskinfo", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        auto filesystem = createFilesystemFromConfig();
        auto attributes = filesystem->getMetadata();

        for (const auto& e : attributes)
            fmt::print("{}={}\n", e.first, quote(e.second));
    }
    catch (const FilesystemException& e)
    {
        Error() << e.message;
    }

    return 0;
}
