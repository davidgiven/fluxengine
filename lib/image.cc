#include "globals.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

Geometry guessGeometry(const SectorSet& sectors)
{
	Geometry g;
	sectors.calculateSize(g.tracks, g.heads, g.sectors, g.sectorSize);
	return g;
}

void readSectorsFromFile(SectorSet& sectors, const Geometry& geometry,
		const std::string& filename)
{
    std::ifstream inputFile(filename, std::ios::in | std::ios::binary);
    if (!inputFile.is_open())
		Error() << "cannot open input file";

    size_t headSize = geometry.sectors * geometry.sectorSize;
    size_t trackSize = headSize * geometry.heads;

    std::cout << fmt::format("{} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
					geometry.tracks, geometry.heads,
					geometry.sectors, geometry.sectorSize,
					geometry.tracks * trackSize / 1024)
			  << std::endl;

	std::vector<uint8_t> data(geometry.sectorSize);
	for (int track = 0; track < geometry.tracks; track++)
	{
		for (int head = 0; head < geometry.heads; head++)
		{
			for (int sectorId = 0; sectorId < geometry.sectors; sectorId++)
			{
				inputFile.seekg(track*trackSize + head*headSize + sectorId*geometry.sectorSize, std::ios::beg);
				inputFile.read((char*) &data[0], geometry.sectorSize);

				sectors[{track, head, sectorId}].reset(
					new Sector(Sector::OK, track, head, sectorId, data));
			}
		}
	}
}

void writeSectorsToFile(const SectorSet& sectors, const Geometry& geometry,
		const std::string& filename)
{
	/* Emit the map. */

	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;
	std::cout << "H.SS Tracks --->" << std::endl;
	for (int head = 0; head < geometry.heads; head++)
	{
		for (int sectorId = 0; sectorId < geometry.sectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < geometry.tracks; track++)
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

    size_t headSize = geometry.sectors * geometry.sectorSize;
    size_t trackSize = headSize * geometry.heads;

    std::cout << fmt::format("{} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
					geometry.tracks, geometry.heads,
					geometry.sectors, geometry.sectorSize,
					geometry.tracks * trackSize / 1024)
			  << std::endl;

    std::ofstream outputFile(filename, std::ios::out | std::ios::binary);
    if (!outputFile.is_open())
		Error() << "cannot open output file";

	for (int track = 0; track < geometry.tracks; track++)
	{
		for (int head = 0; head < geometry.heads; head++)
		{
			for (int sectorId = 0; sectorId < geometry.sectors; sectorId++)
			{
				auto sector = sectors[{track, head, sectorId}];
				if (sector)
				{
					outputFile.seekp(sector->track*trackSize + sector->side*headSize + sector->sector*geometry.sectorSize, std::ios::beg);
					outputFile.write((const char*) &sector->data[0], sector->data.size());
				}
			}
		}
	}
}
