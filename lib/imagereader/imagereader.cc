#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/core/utils.h"
#include "lib/config/proto.h"
#include "lib/data/image.h"
#include "lib/data/layout.h"
#include "lib/config/config.pb.h"
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

        case IMAGETYPE_DSK:
            return ImageReader::createDSKImageReader(config);

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
	static const std::map<std::string, std::function<void(void)>> formats =
	{
		{".adf",      [&]() { proto->mutable_img(); }},
		{".jv3",      [&]() { proto->mutable_jv3(); }},
		{".d64",      [&]() { proto->mutable_d64(); }},
		{".d81",      [&]() { proto->mutable_img(); }},
		{".d88",      [&]() { proto->mutable_d88(); }},
		{".dim",      [&]() { proto->mutable_dim(); }},
		{".diskcopy", [&]() { proto->mutable_diskcopy(); }},
		{".dsk",      [&]() { proto->mutable_dsk(); }},
		{".fdi",      [&]() { proto->mutable_fdi(); }},
		{".imd",      [&]() { proto->mutable_imd(); }},
		{".img",      [&]() { proto->mutable_img(); }},
		{".st",       [&]() { proto->mutable_img(); }},
		{".nfd",      [&]() { proto->mutable_nfd(); }},
		{".nsi",      [&]() { proto->mutable_nsi(); }},
		{".td0",      [&]() { proto->mutable_td0(); }},
		{".vgi",      [&]() { proto->mutable_img(); }},
		{".xdf",      [&]() { proto->mutable_img(); }},
	};
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
