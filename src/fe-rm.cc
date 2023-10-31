#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/proto.h"
#include "fluxengine.h"
#include "lib/vfs/vfs.h"
#include "lib/utils.h"
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
