#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include <fmt/format.h>
#include <fstream>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "brother.img");

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed records.");

static IntFlag retries(
	{ "--retries" },
	"How many times to retry each track in the event of a read failure.",
	5);

#define SECTOR_COUNT 12
#define TRACK_COUNT 78

int main(int argc, const char* argv[])
{
	setReaderDefaults(0, 81, 0, 0);
    Flag::parseFlags(argc, argv);

	bool failures = false;
    std::vector<std::unique_ptr<Sector>> allSectors;
    for (auto& track : readTracks())
    {
		std::map<int, std::unique_ptr<Sector>> readSectors;
		for (int retry = ::retries; retry >= 0; retry--)
		{
			Fluxmap& fluxmap = track->read();

			nanoseconds_t clockPeriod = fluxmap.guessClock();
			std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

			auto bitmap = decodeFluxmapToBits(fluxmap, clockPeriod);
			std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

			auto records = decodeBitsToRecordsBrother(bitmap);
			std::cout << records.size() << " records." << std::endl;

			auto sectors = parseRecordsToSectorsBrother(records);
			std::cout << "       " << sectors.size() << " sectors; ";

			for (auto& sector : sectors)
			{
				if ((sector->sector < SECTOR_COUNT) && (sector->track < TRACK_COUNT))
				{
					auto& replacing = readSectors[sector->sector];
					if (sector->status == Sector::OK)
						replacing = std::move(sector);
					else
					{
						if (!replacing || (replacing->status == Sector::OK))
							replacing = std::move(sector);
					}
				}
			}

			bool hasBadSectors = false;
			for (int i=0; i<SECTOR_COUNT; i++)
			{
				auto& sector = readSectors[i];
				if (!sector || (sector->status != Sector::OK))
				{
					std::cout << std::endl
							  << "       Failed to read sector " << i << "; ";
					hasBadSectors = true;
				}
			}

			if (hasBadSectors)
				failures = false;

			if (dumpRecords && (!hasBadSectors || (retry == 0)))
			{
				std::cout << "\nRaw records follow:\n\n";
				for (auto& record : records)
				{
					hexdump(std::cout, record);
					std::cout << std::endl;
				}
			}

			if (!hasBadSectors)
				break;

			std::cout << std::endl
					  << "       " << retry << " retries remaining" << std::endl;
			track->forceReread();
		}

        int size = 0;
		bool printedTrack = false;
		for (int sectorId = 0; sectorId < SECTOR_COUNT; sectorId++)
		{
			auto& sector = readSectors[sectorId];
			if (sector)
			{
				if (!printedTrack)
				{
					std::cout << "       logical track " << sector->track << "; ";
					printedTrack = true;
				}

				size += sector->data.size();
				allSectors.push_back(std::move(sector));
			}
        }
        std::cout << size << " bytes decoded." << std::endl;

    }

    writeSectorsToFile(allSectors, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
    return 0;
}

