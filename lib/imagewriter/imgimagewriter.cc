#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "lib/config.pb.h"
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

	void writeImage(const SectorSet& sectors)
	{
		unsigned autoTracks;
		unsigned autoSides;
		unsigned autoSectors;
		unsigned autoBytes;
		sectors.calculateSize(autoTracks, autoSides, autoSectors, autoBytes);

		int tracks = _config.img().has_tracks() ? _config.img().tracks() : autoTracks;
		int sides = _config.img().has_sides() ? _config.img().sides() : autoSides;

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		for (int track = 0; track < tracks; track++)
		{
			for (int side = 0; side < sides; side++)
			{
				ImgInputOutputProto::FormatProto format;
				getTrackFormat(format, track, side);

				int numSectors = format.has_sectors() ? format.sectors() : autoSectors;
				int sectorSize = format.has_sector_size() ? format.sector_size() : autoBytes;

				for (int sectorId = 0; sectorId < numSectors; sectorId++)
				{
					const auto& sector = sectors.get(track, side, sectorId);
					if (sector)
						sector->data.slice(0, sectorSize).writeTo(outputFile);
					else
						outputFile.seekp(sectorSize, std::ios::cur);
				}
			}
		}

		std::cout << fmt::format("written {} tracks, {} sides, {} kB total\n",
						tracks, sides,
						outputFile.tellp() / 1024);
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

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(config));
}
