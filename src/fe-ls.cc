#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "sector.h"
#include "proto.h"
#include "readerwriter.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "fluxengine.h"
#include "lib/sectorinterface.h"
#include "lib/vfs/vfs.h"
#include <google/protobuf/text_format.h>
#include <fstream>

static FlagGroup flags;

static StringFlag image({"-i", "--image"},
    "image to work on",
    "",
    [](const auto& value)
    {
        ImageReader::updateConfigForFilename(
            config.mutable_image_reader(), value);
    });

static StringFlag directory({"-d", "--directory"}, "directory to list", "");

static char fileTypeChar(FileType fileType)
{
    switch (fileType)
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

    std::shared_ptr<SectorInterface> sectorInterface;

    auto reader = ImageReader::create(config.image_reader());
    std::shared_ptr<Image> image(std::move(reader->readImage()));
    sectorInterface = std::make_shared<ImageSectorInterface>(image);
    auto filesystem =
        Filesystem::createFilesystem(config.filesystem(), sectorInterface);

    auto path = parsePath(directory);
    auto files = filesystem->list(path);

    int maxlen = 0;
    for (const auto& dirent : files)
        maxlen = std::max(maxlen, (int)dirent->filename.size());

	uint32_t total = 0;
    for (const auto& dirent : files)
    {
        fmt::print("{} {:{}}  {:6}\n",
            fileTypeChar(dirent->fileType),
            dirent->filename,
            maxlen,
            dirent->length);
		total += dirent->length;
    }
    fmt::print("({} files, {} bytes)\n", files.size(), total);

    return 0;
}
