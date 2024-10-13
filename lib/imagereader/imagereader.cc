#include "lib/core/globals.h"
#include "lib/config.h"
#include "lib/flags.h"
#include "lib/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/core/utils.h"
#include "lib/proto.h"
#include "lib/image.h"
#include "lib/layout.h"
#include "lib/config.pb.h"
#include "lib/core/logger.h"
#include <algorithm>
#include <ctype.h>

std::unique_ptr<ImageReader> ImageReader::create(Config& config)
{
    if (!config.hasImageReader())
        error("no image reader configured");
    return create(config->image_reader());
}

std::unique_ptr<ImageReader> ImageReader::create(const ImageReaderProto& config)
{
    switch (config.type())
    {
        case IMAGETYPE_DIM:
            return ImageReader::createDimImageReader(config);

        case IMAGETYPE_D88:
            return ImageReader::createD88ImageReader(config);

        case IMAGETYPE_FDI:
            return ImageReader::createFdiImageReader(config);

        case IMAGETYPE_IMD:
            return ImageReader::createIMDImageReader(config);

        case IMAGETYPE_IMG:
            return ImageReader::createImgImageReader(config);

        case IMAGETYPE_DISKCOPY:
            return ImageReader::createDiskCopyImageReader(config);

        case IMAGETYPE_JV3:
            return ImageReader::createJv3ImageReader(config);

        case IMAGETYPE_D64:
            return ImageReader::createD64ImageReader(config);

        case IMAGETYPE_NFD:
            return ImageReader::createNFDImageReader(config);

        case IMAGETYPE_NSI:
            return ImageReader::createNsiImageReader(config);

        case IMAGETYPE_TD0:
            return ImageReader::createTd0ImageReader(config);

        default:
            error("bad input file config");
            return std::unique_ptr<ImageReader>();
    }
}

ImageReader::ImageReader(const ImageReaderProto& config): _config(config) {}

std::unique_ptr<Image> ImageReader::readMappedImage()
{
    auto rawImage = readImage();

    if (!_config.filesystem_sector_order())
        return rawImage;

    log("READER: converting from filesystem sector order to disk order");
    std::set<std::shared_ptr<const Sector>> sectors;
    for (const auto& e : *rawImage)
    {
        auto trackLayout =
            Layout::getLayoutOfTrack(e->logicalTrack, e->logicalSide);
        auto newSector = std::make_shared<Sector>();
        *newSector = *e;
        newSector->logicalSector =
            trackLayout->filesystemToNaturalSectorMap.at(e->logicalSector);
        sectors.insert(newSector);
    }

    return std::make_unique<Image>(sectors);
}
