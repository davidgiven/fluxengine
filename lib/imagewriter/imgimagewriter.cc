#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "image.h"
#include "lib/config.pb.h"
#include "imginputoutpututils.h"
#include "fmt/format.h"
#include "logger.h"
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

		for (const auto& p : getTrackOrdering(_config.img(), tracks, sides))
		{
			int track = p.first;
			int side = p.second;

			ImgInputOutputProto::TrackdataProto trackdata;
			getTrackFormat(_config.img(), trackdata, track, side);

			auto sectors = getSectors(trackdata, geometry.numSectors);
			if (sectors.empty())
			{
				int maxSector = geometry.firstSector + geometry.numSectors - 1;
				for (int i=geometry.firstSector; i<=maxSector; i++)
					sectors.push_back(i);
			}

			int sectorSize = trackdata.has_sector_size() ? trackdata.sector_size() : geometry.sectorSize;

			for (int sectorId : sectors)
			{
				const auto& sector = image.get(track, side, sectorId);
				if (sector)
					sector->data.slice(0, sectorSize).writeTo(outputFile);
				else
					outputFile.seekp(sectorSize, std::ios::cur);
			}
		}

		Logger() << fmt::format("IMG: wrote {} tracks, {} sides, {} kB total",
						tracks, sides,
						outputFile.tellp() / 1024);
	}

	std::vector<unsigned> getSectors(const ImgInputOutputProto::TrackdataProto& trackdata, unsigned numSectors)
	{
		std::vector<unsigned> sectors;
		switch (trackdata.sectors_oneof_case())
		{
			case ImgInputOutputProto::TrackdataProto::SectorsOneofCase::kSectors:
			{
				for (int sectorId : trackdata.sectors().sector())
					sectors.push_back(sectorId);
				break;
			}

			case ImgInputOutputProto::TrackdataProto::SectorsOneofCase::kSectorRange:
			{
				int sectorId = trackdata.sector_range().start_sector();
				if (trackdata.sector_range().has_sector_count())
					numSectors = trackdata.sector_range().sector_count();
				for (int i=0; i<numSectors; i++)
					sectors.push_back(sectorId + i);
				break;
			}

			default:
				break;
		}
		return sectors;
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(config));
}
