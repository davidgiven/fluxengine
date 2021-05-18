#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagereader/imagereader.h"
#include "lib/config.pb.h"
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

	SectorSet readImage()
	{
        std::ifstream inputFile(_config.filename(), std::ios::in | std::ios::binary);
        if (!inputFile.is_open())
            Error() << "cannot open input file";

        SectorSet sectors;
        for (int track = 0; track < _config.img().tracks(); track++)
        {
            for (int side = 0; side < _config.img().sides(); side++)
            {
				ImgInputOutputProto::FormatProto format;
				getTrackFormat(format, track, side);

                for (int sectorId = 0; sectorId < format.sectors(); sectorId++)
                {
                    Bytes data(format.sector_size());
                    inputFile.read((char*) data.begin(), data.size());

                    std::unique_ptr<Sector>& sector = sectors.get(track, side, sectorId);
                    sector.reset(new Sector);
                    sector->status = Sector::OK;
                    sector->logicalTrack = track;
					sector->physicalTrack = track;
                    sector->logicalSide = sector->physicalSide = side;
                    sector->logicalSector = sectorId;
                    sector->data = data;
                }
            }

			if (inputFile.eof())
				break;
        }

        std::cout << fmt::format("reading {} tracks, {} sides, {} kB total\n",
                        _config.img().tracks(), _config.img().sides(),
						inputFile.tellg() / 1024);
        return sectors;
	}

private:
	void getTrackFormat(ImgInputOutputProto::FormatProto& format, unsigned track, unsigned side)
	{
		format.Clear();
		for (const ImgInputOutputProto::FormatProto& f : _config.img().format())
		{
			if (f.has_track() && (f.track() != track))
				continue;
			if (f.has_side() && (f.side() != side))
				continue;

			format.MergeFrom(f);
		}
	}
};

std::unique_ptr<ImageReader> ImageReader::createImgImageReader(
	const ImageReaderProto& config)
{
    return std::unique_ptr<ImageReader>(new ImgImageReader(config));
}

