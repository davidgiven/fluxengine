#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "utils.h"
#include "proto.h"
#include "image.h"
#include "lib/layout.h"
#include "lib/config.pb.h"
#include "lib/logger.h"
#include <algorithm>
#include <ctype.h>

std::unique_ptr<ImageReader> ImageReader::create(const ImageReaderProto& config)
{
    switch (config.type())
    {
        case ImageReaderProto::DIM:
            return ImageReader::createDimImageReader(config);

        case ImageReaderProto::D88:
            return ImageReader::createD88ImageReader(config);

        case ImageReaderProto::FDI:
            return ImageReader::createFdiImageReader(config);

        case ImageReaderProto::IMD:
            return ImageReader::createIMDImageReader(config);

        case ImageReaderProto::IMG:
            return ImageReader::createImgImageReader(config);

        case ImageReaderProto::DISKCOPY:
            return ImageReader::createDiskCopyImageReader(config);

        case ImageReaderProto::JV3:
            return ImageReader::createJv3ImageReader(config);

        case ImageReaderProto::D64:
            return ImageReader::createD64ImageReader(config);

        case ImageReaderProto::NFD:
            return ImageReader::createNFDImageReader(config);

        case ImageReaderProto::NSI:
            return ImageReader::createNsiImageReader(config);

        case ImageReaderProto::TD0:
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
