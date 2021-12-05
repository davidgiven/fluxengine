#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
#include "proto.h"
#include "lib/config.pb.h"
#include "imagereader/imagereaderimpl.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

// reader based on this partial documentation of the DIM format:
// https://www.pc98.org/project/doc/dim.html

class DimImageReader : public ImageReader
{
public:
	DimImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	std::unique_ptr<Image> readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes header(256);
        inputFile.read((char*) header.begin(), header.size());
        if (header.slice(0xAB, 13) != Bytes("DIFC HEADER  "))
            Error() << "DIM: could not find DIM header, is this a DIM file?";

        // the DIM header technically has a bit field for sectors present,
        // however it is currently ignored by this reader

        char mediaByte = header[0];
        int tracks;
        int sectorsPerTrack;
        int sectorSize;
        switch (mediaByte) {
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
                Error() << "DIM: unsupported media byte";
                break;
        }

        std::unique_ptr<Image> image(new Image);
		int trackCount = 0;
        for (int track = 0; track < tracks; track++)
        {
			if (inputFile.eof())
				break;
			int physicalCylinder = track;

            for (int side = 0; side < 2; side++)
            {
                std::vector<unsigned> sectors;
                for (int i = 0; i < sectorsPerTrack; i++)
                    sectors.push_back(i + 1);

                for (int sectorId : sectors)
                {
                    Bytes data(sectorSize);
                    inputFile.read((char*) data.begin(), data.size());

					const auto& sector = image->put(physicalCylinder, side, sectorId);
                    sector->status = Sector::OK;
                    sector->logicalTrack = track;
					sector->physicalCylinder = physicalCylinder;
                    sector->logicalSide = sector->physicalHead = side;
                    sector->logicalSector = sectorId;
                    sector->data = data;
                }
            }

			trackCount++;
        }

        if (config.encoder().format_case() == EncoderProto::FormatCase::FORMAT_NOT_SET)
        {
            auto ibm = config.mutable_encoder()->mutable_ibm();
            auto trackdata = ibm->add_trackdata();
            trackdata->set_clock_rate_khz(500);
            auto sectors = trackdata->mutable_sectors();
            switch (mediaByte) {
                case 0x00:
                    std::cout << "DIM: automatically setting format to 1.2MB (1024 byte sectors)\n";
                    config.mutable_cylinders()->set_end(76);
                    trackdata->set_track_length_ms(167);
                    trackdata->set_sector_size(1024);
                    for (int i = 0; i < 9; i++)
                        sectors->add_sector(i);
                    break;
                case 0x02:
                    std::cout << "DIM: automatically setting format to 1.2MB (512 byte sectors)\n";
                    trackdata->set_track_length_ms(167);
                    trackdata->set_sector_size(512);
                    for (int i = 0; i < 15; i++)
                        sectors->add_sector(i);
                    break;
                case 0x03:
                    std::cout << "DIM: automatically setting format to 1.44MB\n";
                    trackdata->set_track_length_ms(200);
                    trackdata->set_sector_size(512);
                    for (int i = 0; i < 18; i++)
                        sectors->add_sector(i);
                    break;
                default:
                    Error() << fmt::format("DIM: unknown media byte 0x%02x, could not determine write profile automatically", mediaByte);
                    break;
            }
        }

		image->calculateSize();
		const Geometry& geometry = image->getGeometry();
        std::cout << fmt::format("DIM: read {} tracks, {} sides, {} kB total\n",
                        geometry.numTracks, geometry.numSides,
						((int)inputFile.tellg() - 256) / 1024);
        return image;
	}

};

std::unique_ptr<ImageReader> ImageReader::createDimImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new DimImageReader(config));
}

