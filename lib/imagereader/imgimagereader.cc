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

class ImgImageReader : public ImageReader
{
public:
	ImgImageReader(const ImageReaderProto& config):
		ImageReader(config)
	{}

	Image readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

		if (!_config.img().tracks() || !_config.img().sides())
			Error() << "IMG: bad configuration; did you remember to set the tracks, sides and trackdata fields?";

        Image image;
		int trackCount = 0;
        for (int track = 0; track < _config.img().tracks(); track++)
        {
			if (inputFile.eof())
				break;
			int physicalCylinder = track * _config.img().physical_step() + _config.img().physical_offset();

            for (int side = 0; side < _config.img().sides(); side++)
            {
				ImgInputOutputProto::TrackdataProto trackdata;
				getTrackFormat(_config.img(), trackdata, track, side);

                for (int sectorId = 0; sectorId < trackdata.sectors(); sectorId++)
                {
                    Bytes data(trackdata.sector_size());
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
        std::cout << fmt::format("IMG: read {} tracks, {} sides, {} kB total\n",
                        geometry.numTracks, geometry.numSides,
						inputFile.tellg() / 1024);
        return image;
	}
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}

