#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/sector.h"
#include "lib/imagereader/imagereader.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/config/proto.h"
#include "lib/config/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the DIM format:
// https://www.pc98.org/project/doc/dim.html

class DimImageReader : public ImageReader
{
public:
    DimImageReader(const ImageReaderProto& config): ImageReader(config) {}

    std::unique_ptr<Image> readImage() override
    {
        std::ifstream inputFile(
            _config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            error("cannot open input file");

        Bytes header(256);
        inputFile.read((char*)header.begin(), header.size());
        if (header.slice(0xAB, 13) != Bytes("DIFC HEADER  "))
            error("DIM: could not find DIM header, is this a DIM file?");

        // the DIM header technically has a bit field for sectors present,
        // however it is currently ignored by this reader

        char mediaByte = header[0];
        int tracks;
        int sectorsPerTrack;
        int sectorSize;
        switch (mediaByte)
        {
            case 0:
                tracks = 77;
                sectorsPerTrack = 8;
                sectorSize = 1024;
                break;
            case 1:
                tracks = 80;
                sectorsPerTrack = 9;
                sectorSize = 1024;
                break;
            case 2:
                tracks = 80;
                sectorsPerTrack = 15;
                sectorSize = 512;
                break;
            case 3:
                tracks = 80;
                sectorsPerTrack = 18;
                sectorSize = 512;
                break;
            default:
                error("DIM: unsupported media byte");
                break;
        }

        std::unique_ptr<Image> image(new Image);
        int trackCount = 0;
        for (int track = 0; track < tracks; track++)
        {
            if (inputFile.eof())
                break;

            for (int side = 0; side < 2; side++)
            {
                std::vector<unsigned> sectors;
                for (int i = 0; i < sectorsPerTrack; i++)
                    sectors.push_back(i + 1);

                for (int sectorId : sectors)
                {
                    Bytes data(sectorSize);
                    inputFile.read((char*)data.begin(), data.size());

                    const auto& sector = image->put(track, side, sectorId);
                    sector->status = Sector::OK;
                    sector->data = data;
                }
            }

            trackCount++;
        }

        auto layout = _extraConfig.mutable_layout();
        if (globalConfig()->encoder().format_case() ==
            EncoderProto::FormatCase::FORMAT_NOT_SET)
        {
            auto ibm = _extraConfig.mutable_encoder()->mutable_ibm();
            auto trackdata = ibm->add_trackdata();
            trackdata->set_target_clock_period_us(2);

            auto layoutdata = layout->add_layoutdata();
            auto physical = layoutdata->mutable_physical();
            switch (mediaByte)
            {
                case 0x00:
                    log("DIM: automatically setting format to 1.2MB "
                        "(1024 byte sectors)");
                    trackdata->set_target_rotational_period_ms(167);
                    layoutdata->set_sector_size(1024);
                    for (int i = 0; i < 9; i++)
                        physical->add_sector(i);
                    break;
                case 0x02:
                    log("DIM: automatically setting format to 1.2MB "
                        "(512 byte sectors)");
                    trackdata->set_target_rotational_period_ms(167);
                    layoutdata->set_sector_size(512);
                    for (int i = 0; i < 15; i++)
                        physical->add_sector(i);
                    break;
                case 0x03:
                    log("DIM: automatically setting format to 1.44MB");
                    trackdata->set_target_rotational_period_ms(200);
                    layoutdata->set_sector_size(512);
                    for (int i = 0; i < 18; i++)
                        physical->add_sector(i);
                    break;
                default:
                    error(
                        "DIM: unknown media byte 0x{:02x}, could not determine "
                        "write profile automatically",
                        mediaByte);
                    break;
            }

            _extraConfig.mutable_decoder()->mutable_ibm();
        }

        image->calculateSize();
        const Geometry& geometry = image->getGeometry();
        log("DIM: read {} tracks, {} sides, {} kB total",
            geometry.numTracks,
            geometry.numSides,
            ((int)inputFile.tellg() - 256) / 1024);

        layout->set_tracks(geometry.numTracks);
        layout->set_sides(geometry.numSides);

        return image;
    }
};

std::unique_ptr<ImageReader> ImageReader::createDimImageReader(
    const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new DimImageReader(config));
}
