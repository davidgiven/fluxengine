#include "globals.h"
#include "flags.h"
#include "dataspec.h"
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
	ImgImageWriter(const OutputFileProto& config):
		ImageWriter(config)
	{}

	void writeImage(const SectorSet& sectors)
	{
		unsigned numCylinders;
		unsigned numHeads;
		unsigned numSectors;
		unsigned numBytes;
		sectors.calculateSize(numCylinders, numHeads, numSectors, numBytes);

		size_t headSize = numSectors * numBytes;
		size_t trackSize = headSize * numHeads;

		std::cout << fmt::format("writing {} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
						numCylinders, numHeads,
						numSectors, numBytes,
						numCylinders * trackSize / 1024)
				<< std::endl;

		std::ofstream outputFile(_config.filename(), std::ios::out | std::ios::binary);
		if (!outputFile.is_open())
			Error() << "cannot open output file";

		for (int track = 0; track < numCylinders; track++)
		{
			for (int head = 0; head < numHeads; head++)
			{
				for (int sectorId = 0; sectorId < numSectors; sectorId++)
				{
					const auto& sector = sectors.get(track, head, sectorId);
					if (sector)
					{
						outputFile.seekp(sector->logicalTrack*trackSize + sector->logicalSide*headSize + sector->logicalSector*numBytes, std::ios::beg);
						sector->data.slice(0, numBytes).writeTo(outputFile);
					}
				}
			}
		}
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const OutputFileProto& config)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(config));
}
