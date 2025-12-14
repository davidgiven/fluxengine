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
