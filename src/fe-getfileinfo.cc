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
#include "lib/vfs/sectorinterface.h"
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

int mainGetFileInfo(int argc, const char* argv[])
{
    if (argc == 1)
        showProfiles("ls", formats);
    flags.parseFlagsWithConfigFiles(argc, argv, formats);

    std::shared_ptr<SectorInterface> sectorInterface;

    auto reader = ImageReader::create(config.image_reader());
    std::shared_ptr<Image> image(std::move(reader->readImage()));
    sectorInterface = SectorInterface::createImageSectorInterface(image);
    auto filesystem =
        Filesystem::createFilesystem(config.filesystem(), sectorInterface);

    Path path(directory);
	auto attributes = filesystem->getMetadata(path);

	for (const auto& e : attributes)
		fmt::print("{} = {}\n", e.first, e.second);

    return 0;
}
