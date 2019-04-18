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

	for (int track = 0; track < geometry.tracks; track++)
	{
		for (int head = 0; head < geometry.heads; head++)
		{
			for (int sectorId = 0; sectorId < geometry.sectors; sectorId++)
			{
				inputFile.seekg(track*trackSize + head*headSize + sectorId*geometry.sectorSize, std::ios::beg);

				Bytes data(geometry.sectorSize);
				inputFile.read((char*) data.begin(), geometry.sectorSize);

				Sector* sector = sectors.get(track, head, sectorId) = new Sector();
				sector->status = Sector::OK;
				sector->logicalTrack = sector->physicalTrack = track;
				sector->logicalSide = sector->physicalSide = head;
				sector->logicalSector = sectorId;
				sector->data = data;
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
				Sector* sector = sectors.get(track, head, sectorId);
				if (!sector)
				{
					std::cout << 'X';
					missingSectors++;
				}
				else
				{
					switch (sector->status)
					{
						case Sector::OK:
                            std::cout << '.';
                            break;

                        case Sector::BAD_CHECKSUM:
                            std::cout << 'B';
                            badSectors++;
                            break;

                        case Sector::CONFLICT:
                            std::cout << 'C';
                            badSectors++;
                            break;

                        default:
                            std::cout << '?';
                            break;
                    }
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
				auto sector = sectors.get(track, head, sectorId);
				if (sector)
				{
					outputFile.seekp(sector->logicalTrack*trackSize + sector->logicalSide*headSize + sector->logicalSector*geometry.sectorSize, std::ios::beg);
					outputFile.write((const char*) sector->data.cbegin(), sector->data.size());
				}
			}
		}
	}
}
