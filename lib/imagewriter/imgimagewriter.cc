#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "image.h"
#include "lib/config.pb.h"
#include "imagereader/imagereaderimpl.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageWriter : public ImageWriter
{
public:
	ImgImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{}

	void writeImage(const Image& image)
	{
		const Geometry geometry = image.getGeometry();

		int tracks = _config.img().has_tracks() ? _config.img().tracks() : geometry.numTracks;
		int sides = _config.img().has_sides() ? _config.img().sides() : geometry.numSides;

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		for (int track = 0; track < tracks; track++)
		{
			for (int side = 0; side < sides; side++)
			{
				ImgInputOutputProto::TrackdataProto trackdata;
				getTrackFormat(_config.img(), trackdata, track, side);

				int numSectors = trackdata.has_sectors() ? trackdata.sectors() : geometry.numSectors;
				int sectorSize = trackdata.has_sector_size() ? trackdata.sector_size() : geometry.sectorSize;

				for (int sectorId = 0; sectorId < numSectors; sectorId++)
				{
					const auto& sector = image.get(track, side, sectorId);
					if (sector)
						sector->data.slice(0, sectorSize).writeTo(outputFile);
					else
						outputFile.seekp(sectorSize, std::ios::cur);
				}
			}
		}

		std::cout << fmt::format("IMG: wrote {} tracks, {} sides, {} kB total\n",
						tracks, sides,
						outputFile.tellp() / 1024);
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(config));
}
