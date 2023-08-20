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
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

static StringFlag volumeName({"-n", "--name"}, "volume name", "FluxEngine");
static SettableFlag quick({"-q", "--quick"},
    "perform quick format (requires the disk to be previously formatted)");

int mainFormat(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("format", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        auto filesystem = Filesystem::createFilesystemFromConfig();
        filesystem->create(quick, volumeName);
        filesystem->flushChanges();
    }
    catch (const FilesystemException& e)
    {
        error("{}", e.message);
    }

    return 0;
}
