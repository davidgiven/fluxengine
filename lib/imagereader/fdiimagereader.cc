#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "proto.h"
#include "logger.h"
#include "mapper.h"
#include "lib/config.pb.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the FDI format:
// https://www.pc98.org/project/doc/hdi.html

class FdiImageReader : public ImageReader
{
public:
    FdiImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage()
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes header(32);
        inputFile.read((char*)header.begin(), header.size());
        ByteReader headerReader(header);
        if (headerReader.seek(0).read_le32() != 0)
            Error() << "FDI: could not find FDI header, is this a FDI file?";

        // we currently don't use fddType but it could be used to automatically
        // select profile parameters in the future
        //
        int fddType = headerReader.seek(4).read_le32();
        int headerSize = headerReader.seek(0x08).read_le32();
        int sectorSize = headerReader.seek(0x10).read_le32();
        int sectorsPerTrack = headerReader.seek(0x14).read_le32();
        int sides = headerReader.seek(0x18).read_le32();
        int tracks = headerReader.seek(0x1c).read_le32();

        inputFile.seekg(headerSize);

        std::unique_ptr<Image> image(new Image);
        int trackCount = 0;
        for (int track = 0; track < tracks; track++)
        {
            if (inputFile.eof())
                break;

            for (int side = 0; side < sides; side++)
            {
                std::vector<unsigned> sectors;
                for (int i = 0; i < sectorsPerTrack; i++)
                    sectors.push_back(i + 1);

                for (int sectorId : sectors)
                {
                    Bytes data(sectorSize);
                    inputFile.read((char*)data.begin(), data.size());

                    const auto& sector =
                        image->put(track, side, sectorId);
                    sector->status = Sector::OK;
                    sector->logicalTrack = track;
                    sector->physicalTrack = Mapper::remapTrackLogicalToPhysical(track);
                    sector->logicalSide = sector->physicalHead = side;
                    sector->logicalSector = sectorId;
                    sector->data = data;
                }
            }

            trackCount++;
        }

        if (config.encoder().format_case() ==
            EncoderProto::FormatCase::FORMAT_NOT_SET)
        {
            auto ibm = config.mutable_encoder()->mutable_ibm();
            auto trackdata = ibm->add_trackdata();
            trackdata->set_target_clock_period_us(2);
            auto sectors = trackdata->mutable_sectors();
            switch (fddType)
            {
                case 0x90:
                    Logger() << "FDI: automatically setting format to 1.2MB "
                                "(1024 byte sectors)";
                    config.mutable_tracks()->set_end(76);
                    trackdata->set_target_rotational_period_ms(167);
                    trackdata->set_sector_size(1024);
                    for (int i = 0; i < 9; i++)
                        sectors->add_sector(i);
                    break;
                case 0x30:
                    Logger() << "FDI: automatically setting format to 1.44MB";
                    trackdata->set_target_rotational_period_ms(200);
                    trackdata->set_sector_size(512);
                    for (int i = 0; i < 18; i++)
                        sectors->add_sector(i);
                    break;
                default:
                    Error() << fmt::format(
                        "FDI: unknown fdd type 0x{:2x}, could not determine "
                        "write profile automatically",
                        fddType);
                    break;
            }
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        Logger() << fmt::format("FDI: read {} tracks, {} sides, {} kB total",
            geometry.numTracks,
            geometry.numSides,
            ((int)inputFile.tellg() - headerSize) / 1024);

        if (!config.has_heads())
        {
            auto* heads = config.mutable_heads();
            heads->set_start(0);
            heads->set_end(geometry.numSides - 1);
        }

        if (!config.has_tracks())
        {
            auto* tracks = config.mutable_tracks();
            tracks->set_start(0);
            tracks->set_end(geometry.numTracks - 1);
        }

        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createFdiImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new FdiImageReader(config));
}
