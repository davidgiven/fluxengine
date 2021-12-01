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

// reader based on this partial documentation of the FDI format:
// https://www.pc98.org/project/doc/hdi.html

class FdiImageReader : public ImageReader
{
public:
	FdiImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	Image readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        Bytes header(32);
        inputFile.read((char*) header.begin(), header.size());
        ByteReader headerReader(header);
        if (headerReader.seek(0).read_le32() != 0)
            Error() << "FDI: could not find FDI header, is this a FDI file?";

        // we currently don't use fddType but it could be used to automatically select
        // profile parameters in the future
        //
        int fddType = headerReader.seek(4).read_le32();
        int headerSize = headerReader.seek(0x08).read_le32();
        int sectorSize = headerReader.seek(0x10).read_le32();
        int sectorsPerTrack = headerReader.seek(0x14).read_le32();
        int sides = headerReader.seek(0x18).read_le32();
        int tracks = headerReader.seek(0x1c).read_le32();

        inputFile.seekg(headerSize);

        Image image;
		int trackCount = 0;
        for (int track = 0; track < tracks; track++)
        {
			if (inputFile.eof())
				break;
			int physicalCylinder = track;

            for (int side = 0; side < sides; side++)
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
        std::cout << fmt::format("FDI: read {} tracks, {} sides, {} kB total\n",
                        geometry.numTracks, geometry.numSides,
						((int)inputFile.tellg() - headerSize) / 1024);
        return image;
	}

};

std::unique_ptr<ImageReader> ImageReader::createFdiImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new FdiImageReader(config));
}

