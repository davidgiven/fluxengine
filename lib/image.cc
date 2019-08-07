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

void readSectorsFromFile(SectorSet& sectors, const ImageSpec& spec)
{
    std::ifstream inputFile(spec.filename, std::ios::in | std::ios::binary);
    if (!inputFile.is_open())
		Error() << "cannot open input file";

    size_t headSize = spec.sectors * spec.bytes;
    size_t trackSize = headSize * spec.heads;

    std::cout << fmt::format("{} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
					spec.cylinders, spec.heads,
					spec.sectors, spec.bytes,
					spec.cylinders * trackSize / 1024)
			  << std::endl;

	for (int track = 0; track < spec.cylinders; track++)
	{
		for (int head = 0; head < spec.heads; head++)
		{
			for (int sectorId = 0; sectorId < spec.sectors; sectorId++)
			{
				inputFile.seekg(track*trackSize + head*headSize + sectorId*spec.bytes, std::ios::beg);

				Bytes data(spec.bytes);
				inputFile.read((char*) data.begin(), spec.bytes);

				std::unique_ptr<Sector>& sector = sectors.get(track, head, sectorId);
				sector.reset(new Sector);
				sector->status = Sector::OK;
				sector->logicalTrack = sector->physicalTrack = track;
				sector->logicalSide = sector->physicalSide = head;
				sector->logicalSector = sectorId;
				sector->data = data;
			}
		}
	}
}

void writeSectorsToFile(const SectorSet& sectors, const ImageSpec& spec)
{
	std::unique_ptr<ImageWriter> writer(ImageWriter::create(sectors, spec));
	writer->adjustGeometry();
	writer->printMap();
	writer->writeImage();
}
