#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
#include "readerwriter.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/imagereader/imagereader.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include "lib/vfs/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include "src/fileutils.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags({ &fileFlags });

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

	auto filesystem = createFilesystemFromConfig();
    auto files = filesystem->list(Path(directory));

    int maxlen = 0;
    for (const auto& dirent : files)
        maxlen = std::max(maxlen, (int)dirent->filename.size());

	uint32_t total = 0;
    for (const auto& dirent : files)
    {
        fmt::print("{} {:{}}  {:6}\n",
            fileTypeChar(dirent->file_type),
            dirent->filename,
            maxlen,
            dirent->length);
		total += dirent->length;
    }
    fmt::print("({} files, {} bytes)\n", files.size(), total);

    return 0;
}
