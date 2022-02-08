#include "globals.h"
#include "flags.h"
#include "sector.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "ldbs.h"
#include "image.h"
#include "lib/config.pb.h"
#include <algorithm>
#include <iostream>
#include <fstream>

static const char LABEL[] = "FluxEngine image";

static void write_and_update_checksum(ByteWriter& bw, uint32_t& checksum, const Bytes& data)
{
	ByteReader br(data);
	while (!br.eof())
	{
		uint32_t i = br.read_be16();
		checksum += i;
		checksum = (checksum >> 1) | (checksum << 31);
		bw.write_be16(i);
	}
}

class DSKImageWriter : public ImageWriter
{
public:
	DSKImageWriter(const ImageWriterProto& config):
		ImageWriter(config)
	{}

	void writeImage(const Image& image)
	{
		const Geometry& geometry = image.getGeometry();

		bool mfm = false;

		switch (geometry.sectorSize)
		{
			case 524:
				/* GCR disk */
				break;

			case 512:
				/* MFM disk */
				mfm = true;
				break;

			default:
				Error() << "this image is not compatible with the DSK format";
		}

		std::cout << "writing DSK image\n"
		          << fmt::format("{} tracks, {} sides, {} sectors, {} bytes per sector; {}\n",
				  		geometry.numTracks, geometry.numSides, geometry.numSectors, geometry.sectorSize,
						mfm ? "MFM" : "GCR");

		auto sectors_per_track = [&](int track) -> int
		{
			if (mfm)
				return geometry.numSectors;

			if (track < 16)
				return 12;
			if (track < 32)
				return 11;
			if (track < 48)
				return 10;
			if (track < 64)
				return 9;
			return 8;
		};

		Bytes data;
		ByteWriter bw(data);

		/* Write the actual sector data. */

		uint32_t dataChecksum = 0;
		uint32_t tagChecksum = 0;
		uint32_t offset = 0x0;
		uint32_t sectorDataStart = offset;
		for (int track = 0; track < geometry.numTracks; track++)
		{
			for (int side = 0; side < geometry.numSides; side++)
			{
				int sectorCount = sectors_per_track(track);
				for (int sectorId = 0; sectorId < sectorCount; sectorId++)
				{
					const auto& sector = image.get(track, side, sectorId);
					if (sector)
					{
						bw.seek(offset);
						write_and_update_checksum(bw, dataChecksum, sector->data.slice(0, 512));
					}
					offset += 512;
				}
			}
		}
		uint32_t sectorDataEnd = offset;
		if (!mfm)
		{
			for (int track = 0; track < geometry.numTracks; track++)
			{
				for (int side = 0; side < geometry.numSides; side++)
				{
					int sectorCount = sectors_per_track(track);
					for (int sectorId = 0; sectorId < sectorCount; sectorId++)
					{
						const auto& sector = image.get(track, side, sectorId);
						if (sector)
						{
							bw.seek(offset);
							write_and_update_checksum(bw, tagChecksum, sector->data.slice(512, 12));
						}
						offset += 12;
					}
				}
			}
		}
		uint32_t tagDataEnd = offset;

		data.writeToFile(_config.filename());
    }
};

std::unique_ptr<ImageWriter> ImageWriter::createDSKImageWriter(
	const ImageWriterProto& config)
{
    return std::unique_ptr<ImageWriter>(new DSKImageWriter(config));
}


