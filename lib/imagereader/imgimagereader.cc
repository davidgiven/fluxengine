#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/config/config.pb.h"
#include "lib/config/layout.pb.h"
#include "lib/config/proto.h"
#include "lib/data/layout.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageReader : public ImageReader
{
public:
    ImgImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage() override
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            error("cannot open input file");

        const auto& layout = globalConfig()->layout();
        if (!layout.tracks() || !layout.sides())
            error(
                "IMG: bad configuration; did you remember to set the "
                "tracks, sides and trackdata fields in the layout?");

        const auto diskLayout = createDiskLayout();
        bool in_filesystem_order = _config.img().filesystem_sector_order();
        std::unique_ptr<Image> image(new Image);

        for (auto& logicalLocation :
            in_filesystem_order ? diskLayout->logicalLocationsInFilesystemOrder
                                : diskLayout->logicalLocations)
        {
            auto& ltl = diskLayout->layoutByLogicalLocation.at(logicalLocation);

            for (unsigned sectorId : in_filesystem_order
                                         ? ltl->filesystemSectorOrder
                                         : ltl->naturalSectorOrder)
            {
                if (inputFile.eof())
                    break;

                Bytes data(ltl->sectorSize);
                inputFile.read((char*)data.begin(), data.size());

                const auto& sector = image->put(
                    logicalLocation.cylinder, logicalLocation.head, sectorId);
                sector->status = Sector::OK;
                sector->data = data;
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        log("IMG: read {} tracks, {} sides, {} kB total from {}",
            geometry.numCylinders,
            geometry.numHeads,
            inputFile.tellg() / 1024,
            _config.filename());
        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}
