#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagereader/imagereader.h"
#include "image.h"
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

	Image readImage()
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

        Image image;
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

					const auto& sector = image.put(physicalCylinder, side, sectorId);
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

		image.calculateSize();
		const Geometry& geometry = image.getGeometry();
        std::cout << fmt::format("DIM: read {} tracks, {} sides, {} kB total\n",
                        geometry.numTracks, geometry.numSides,
						inputFile.tellg() / 1024);
        return image;
	}

};

std::unique_ptr<ImageReader> ImageReader::createDimImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new DimImageReader(config));
}

