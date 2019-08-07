#include "globals.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "fmt/format.h"
#include "flags.h"
#include "dataspec.h"
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
	unsigned numCylinders = spec.cylinders;
	unsigned numHeads = spec.heads;
	unsigned numSectors = spec.sectors;
	unsigned numBytes = spec.bytes;
	if (!spec.initialised)
	{
		sectors.calculateSize(numCylinders, numHeads, numSectors, numBytes);
		std::cout << "Autodetecting output geometry\n";
	}

	/* Emit the map. */

	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;
	std::cout << "H.SS Tracks --->" << std::endl;
	for (int head = 0; head < numHeads; head++)
	{
		for (int sectorId = 0; sectorId < numSectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < numCylinders; track++)
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

    size_t headSize = numSectors * numBytes;
    size_t trackSize = headSize * numHeads;

    std::cout << fmt::format("{} tracks, {} heads, {} sectors, {} bytes per sector, {} kB total",
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
				auto sector = sectors.get(track, head, sectorId);
				if (sector)
				{
					outputFile.seekp(sector->logicalTrack*trackSize + sector->logicalSide*headSize + sector->logicalSector*numBytes, std::ios::beg);
					outputFile.write((const char*) sector->data.cbegin(), sector->data.size());
				}
			}
		}
	}
}
