#include "globals.h"
#include "image.h"
#include "fmt/format.h"
#include <algorithm>
#include <iostream>
#include <fstream>

void writeSectorsToFile(const std::vector<std::unique_ptr<Sector>>& sectors, const std::string& filename)
{
    /* Count the tracks, sides and sectors. */

    int trackCount = 0;
    int sideCount = 0;
    int sectorCount = 0;
    size_t sectorSize = 0;
    for (auto& sector : sectors)
    {
        trackCount = std::max(sector->track+1, trackCount);
        sideCount = std::max(sector->side+1, sideCount);
        sectorCount = std::max(sector->sector+1, sectorCount);
        sectorSize = std::max(sector->data.size(), sectorSize);
    }

    size_t sideSize = sectorCount * sectorSize;
    size_t trackSize = sideSize * sideCount;

    std::cout << fmt::format("{} tracks, {} sides, {} sectors, {} bytes per sector, {} kB total",
					trackCount, sideCount, sectorCount, sectorSize,
					trackCount * sideCount * sectorCount * sectorSize / 1024)
			  << std::endl;

    std::ofstream outputFile(filename, std::ios::out | std::ios::binary);
    if (!outputFile.is_open())
        Error() << "cannot open output file";

    for (auto& sector : sectors)
    {
        outputFile.seekp(sector->track*trackSize + sector->side*sideSize + sector->sector*sectorSize, std::ios::beg);
        outputFile.write((const char*) &sector->data[0], sector->data.size());
    }
}
