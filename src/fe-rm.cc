#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/config/proto.h"
#include "fluxengine.h"
#include "lib/vfs/vfs.h"
#include "lib/core/utils.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

static StringFlag filename({"-p", "--path"}, "filename to remove", "");

int mainRm(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("rm", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        auto filesystem = Filesystem::createFilesystemFromConfig();

        Path path(filename);
        if (path.size() == 0)
            error("filename missing");

        filesystem->deleteFile(path);
        filesystem->flushChanges();
    }
    catch (const FilesystemException& e)
    {
        error("{}", e.message);
    }

    return 0;
}
