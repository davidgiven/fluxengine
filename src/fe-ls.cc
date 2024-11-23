#include "lib/core/globals.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/data/sector.h"
#include "lib/config/proto.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/imagereader/imagereader.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "lib/core/utils.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({&fileFlags});

static StringFlag directory({"-p", "--path"}, "path to list", "");

static char fileTypeChar(FileType file_type)
{
    switch (file_type)
    {
        case TYPE_FILE:
            return ' ';

        case TYPE_DIRECTORY:
            return 'D';

        default:
            return '?';
    }
}

int mainLs(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("ls", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    try
    {
        auto filesystem = Filesystem::createFilesystemFromConfig();
        auto files = filesystem->list(Path(directory));

        int maxlen = 0;
        for (const auto& dirent : files)
            maxlen = std::max(maxlen, (int)quote(dirent->filename).size());

        uint32_t total = 0;
        for (const auto& dirent : files)
        {
            fmt::print("{} {:{}}  {:6} {:4} {}\n",
                fileTypeChar(dirent->file_type),
                quote(dirent->filename),
                maxlen + 2,
                dirent->length,
                dirent->mode,
                dirent->attributes[Filesystem::CTIME]);
            total += dirent->length;
        }
        fmt::print("({} files, {} bytes)\n", files.size(), total);
    }
    catch (const FilesystemException& e)
    {
        error("{}", e.message);
    }

    return 0;
}
