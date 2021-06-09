#include "globals.h"
#include "geometry/geometry.h"
#include "sectorset.h"
#include "sector.h"
#include "fmt/format.h"
#include <iostream>
#include <fstream>

void AssemblingGeometryMapper::put(const SectorSet& sectors)
{
	for (const auto& sit : sectors.get())
		put(*sit.second);
}

void AssemblingGeometryMapper::writeCsv(const SectorSet& sectors, const std::string& filename)
{
	std::ofstream f(filename, std::ios::out);
	if (!f.is_open())
		Error() << "cannot open CSV report file";

	f << "\"Physical track\","
		"\"Physical side\","
		"\"Logical track\","
		"\"Logical side\","
		"\"Logical sector\","
		"\"Clock (ns)\","
		"\"Header start (ns)\","
		"\"Header end (ns)\","
		"\"Data start (ns)\","
		"\"Data end (ns)\","
		"\"Raw data address (bytes)\","
		"\"User payload length (bytes)\","
		"\"Status\""
		"\n";

	for (const auto& it : sectors.get())
	{
		const auto& sector = it.second;
		f << fmt::format("{},{},{},{},{},{},{},{},{},{},{},{},{}\n",
			sector->physicalTrack,
			sector->physicalSide,
			sector->logicalTrack,
			sector->logicalSide,
			sector->logicalSector,
			sector->clock,
			sector->headerStartTime,
			sector->headerEndTime,
			sector->dataStartTime,
			sector->dataEndTime,
			sector->position.bytes,
			sector->data.size(),
			Sector::statusToString(sector->status)
		);
	}
}

void AssemblingGeometryMapper::printMap(const SectorSet& sectors)
{
	unsigned numCylinders;
	unsigned numHeads;
	unsigned numSectors;
	unsigned numBytes;
	sectors.calculateSize(numCylinders, numHeads, numSectors, numBytes);

	int badSectors = 0;
	int missingSectors = 0;
	int totalSectors = 0;

	std::cout << "     Tracks -> 1         2         3         ";
	if (numCylinders > 40) {
		std::cout << "4         5         6         7         8";
	}
	std::cout << std::endl;
	std::cout << "H.SS 0123456789012345678901234567890123456789";
	if (numCylinders > 40) {
		std::cout << "01234567890123456789012345678901234567890123";
	}
	std::cout << std::endl;

	for (int head = 0; head < numHeads; head++)
	{
		for (int sectorId = 0; sectorId < numSectors; sectorId++)
		{
			std::cout << fmt::format("{}.{:2} ", head, sectorId);
			for (int track = 0; track < numCylinders; track++)
			{
				const auto& sector = sectors.get(track, head, sectorId);
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
}

