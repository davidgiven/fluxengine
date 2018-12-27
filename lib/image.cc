#include "globals.h"
#include "image.h"
#include "sectorset.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

void writeSectorsToFile(const SectorSet& sectors, const std::string& filename)
{
	/* Emit the map. */

	int numTracks;
	int numHeads;
	int numSectors;
	int sectorSize;
	sectors.calculateSize(numTracks, numHeads, numSectors, sectorSize);

	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;
	std::cout << "H.SS Tracks --->" << std::endl;
	for (int head = 0; head < numHeads; head++)
	{
		for (int sectorId = 0; sectorId < numSectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < numTracks; track++)
			{
				auto sector = sectors[{track, head, sectorId}];
				if (!sector)
				{
					std::cout << '.';
					missingSectors++;
				}
				else if (sector->status == Sector::OK)
					std::cout << 'G';
				else
				{
					badSectors++;
					std::cout << 'B';
				}
				totalSectors++;
			}
			std::cout << std::endl;
		}
	}
	int goodSectors = totalSectors - missingSectors - badSectors;
	if (totalSectors == 0)
		std::cout << "No sectors in output; skipping analysis" << std::endl;
	else
	{
		std::cout << "Good sectors: " << goodSectors << "/" << totalSectors
				  << " (" << (100*goodSectors/totalSectors) << "%)"
				  << std::endl;
		std::cout << "Missing sectors: " << missingSectors << "/" << totalSectors
				  << " (" << (100*missingSectors/totalSectors) << "%)"
				  << std::endl;
		std::cout << "Bad sectors: " << badSectors << "/" << totalSectors
				  << " (" << (100*badSectors/totalSectors) << "%)"
				  << std::endl;
    }

    size_t headSize = numSectors * sectorSize;
    size_t trackSize = headSize * numHeads;

    std::cout << fmt::format("{} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
					numTracks, numHeads, numSectors, sectorSize,
					numTracks * trackSize / 1024)
			  << std::endl;

    std::ofstream outputFile(filename, std::ios::out | std::ios::binary);
    if (!outputFile.is_open())
		Error() << "cannot open output file";

	for (int track = 0; track < numTracks; track++)
	{
		for (int head = 0; head < numHeads; head++)
		{
			for (int sectorId = 0; sectorId < numSectors; sectorId++)
			{
				auto sector = sectors[{track, head, sectorId}];
				if (sector)
				{
					outputFile.seekp(sector->track*trackSize + sector->side*headSize + sector->sector*sectorSize, std::ios::beg);
					outputFile.write((const char*) &sector->data[0], sector->data.size());
				}
			}
		}
	}
}
