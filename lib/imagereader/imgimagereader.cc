#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "logger.h"
#include "mapper.h"
#include "lib/config.pb.h"
#include "lib/layout.pb.h"
#include "lib/proto.h"
#include "lib/layout.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageReader : public ImageReader
{
public:
    ImgImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage()
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        auto layout = config.layout();
        if (!layout.tracks() || !layout.sides())
            Error() << "IMG: bad configuration; did you remember to set the "
                       "tracks, sides and trackdata fields in the layout?";

        std::unique_ptr<Image> image(new Image);
        for (const auto& p : Layout::getTrackOrdering())
        {
            int track = p.first;
            int side = p.second;

            if (inputFile.eof())
                break;

            auto& trackLayout = Layout::getLayoutOfTrack(track, side);
            for (int sectorId : trackLayout.logicalSectors)
            {
                Bytes data(trackLayout.sectorSize);
                inputFile.read((char*)data.begin(), data.size());

                const auto& sector = image->put(track, side, sectorId);
                sector->status = Sector::OK;
                sector->data = data;
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        Logger() << fmt::format("IMG: read {} tracks, {} sides, {} kB total",
            geometry.numTracks,
            geometry.numSides,
            inputFile.tellg() / 1024);
        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}
