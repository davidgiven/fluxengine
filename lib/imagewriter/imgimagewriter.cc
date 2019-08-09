#include "globals.h"
#include "image.h"
#include "flags.h"
#include "dataspec.h"
#include "sector.h"
#include "sectorset.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

class ImgImageWriter : public ImageWriter
{
public:
	ImgImageWriter(const SectorSet& sectors, const ImageSpec& spec):
		ImageWriter(sectors, spec)
	{}

	void writeImage()
	{
		unsigned numCylinders = spec.cylinders;
		unsigned numHeads = spec.heads;
		unsigned numSectors = spec.sectors;
		unsigned numBytes = spec.bytes;

		size_t headSize = numSectors * numBytes;
		size_t trackSize = headSize * numHeads;

		std::cout << fmt::format("writing {} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
						numCylinders, numHeads,
						numSectors, numBytes,
						numCylinders * trackSize / 1024)
				<< std::endl;

		std::ofstream outputFile(spec.filename, std::ios::out | std::ios::binary);
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
						outputFile.write((const char*) sector->data.cbegin(), sector->data.size());
					}
				}
			}
		}
	}
};

std::unique_ptr<ImageWriter> ImageWriter::createImgImageWriter(
	const SectorSet& sectors, const ImageSpec& spec)
{
    return std::unique_ptr<ImageWriter>(new ImgImageWriter(sectors, spec));
}
