#include "lib/globals.h"
#include "lib/flags.h"
#include "lib/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/image.h"
#include "lib/logger.h"
#include "lib/mapper.h"
#include "lib/config.pb.h"
#include "lib/imginputoutpututils.h"
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

        if (!_config.img().tracks() || !_config.img().sides())
            Error() << "IMG: bad configuration; did you remember to set the "
                       "tracks, sides and trackdata fields?";

        std::unique_ptr<Image> image(new Image);
        for (const auto& p : getTrackOrdering(_config.img()))
        {
            int track = p.first;
            int side = p.second;

            if (inputFile.eof())
                break;

            ImgInputOutputProto::TrackdataProto trackdata;
            getTrackFormat(_config.img(), trackdata, track, side);

            for (int sectorId : getSectors(trackdata))
            {
                Bytes data(trackdata.sector_size());
                inputFile.read((char*)data.begin(), data.size());

                const auto& sector = image->put(track, side, sectorId);
                sector->status = Sector::OK;
                sector->logicalTrack = track;
                sector->physicalTrack = Mapper::remapTrackLogicalToPhysical(track);
                sector->logicalSide = sector->physicalHead = side;
                sector->logicalSector = sectorId;
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

    std::vector<unsigned> getSectors(
        const ImgInputOutputProto::TrackdataProto& trackdata)
    {
        std::vector<unsigned> sectors;
        switch (trackdata.sectors_oneof_case())
        {
            case ImgInputOutputProto::TrackdataProto::SectorsOneofCase::
                kSectors:
            {
                for (int sectorId : trackdata.sectors().sector())
                    sectors.push_back(sectorId);
                break;
            }

            case ImgInputOutputProto::TrackdataProto::SectorsOneofCase::
                kSectorRange:
            {
                int sectorId = trackdata.sector_range().start_sector();
                for (int i = 0; i < trackdata.sector_range().sector_count();
                     i++)
                    sectors.push_back(sectorId + i);
                break;
            }

            default:
                Error() << "no list of sectors provided in track format";
        }
        return sectors;
    }
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}
