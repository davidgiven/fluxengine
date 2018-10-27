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

#define SECTOR_COUNT 12

int main(int argc, const char* argv[])
{
	setReaderDefaults(0, 81, 0, 0);
    Flag::parseFlags(argc, argv);

	bool failures = false;
	std::ofstream o("records.txt");
    std::vector<std::unique_ptr<Sector>> allSectors;
    for (auto& track : readTracks())
    {
		int retries = 5;
	retry:
        Fluxmap& fluxmap = track->read();

        nanoseconds_t clockPeriod = fluxmap.guessClock();
        std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

        auto bitmap = decodeFluxmapToBits(fluxmap, clockPeriod);
        std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

        auto records = decodeBitsToRecordsBrother(bitmap);
        std::cout << records.size() << " records." << std::endl;

        auto sectors = parseRecordsToSectorsBrother(records);
        std::cout << "       " << sectors.size() << " sectors; ";

		std::vector<std::unique_ptr<Sector>> goodSectors(SECTOR_COUNT);
		for (auto& sector : sectors)
		{
			if (sector->status == Sector::OK)
				goodSectors.at(sector->sector) = std::move(sector);
		}

		bool hasBadSectors = false;
		for (int i=0; i<SECTOR_COUNT; i++)
		{
			if (!goodSectors.at(i))
			{
				std::cout << std::endl
						  << "       Failed to read sector " << i << "; ";
				hasBadSectors = true;
			}
		}
		if (hasBadSectors)
		{
			if (retries == 0)
				failures = true;
			else
			{
				std::cout << std::endl
				          << "       " << retries << " retries remaining" << std::endl;
				retries--;
				track->forceReread();
				goto retry;
			}
		}

        int size = 0;
        for (auto& sector : goodSectors)
        {
			if (sector)
			{
				size += sector->data.size();
				allSectors.push_back(std::move(sector));
			}
        }
        std::cout << size << " bytes decoded." << std::endl;

		if (dumpRecords)
		{
			std::cout << "\nRaw records follow:\n\n";
			for (auto& record : records)
			{
				hexdump(std::cout, record);
				std::cout << std::endl;
			}
		}

		for (auto& record : records)
		{
			if ((record.size() >= 260) && (record[0] == 0xdb))
			{
				for (int i=1; i<260; i++)
					o << fmt::format("{:02x}", record[i]);
				o << std::endl;
			}
		}
    }

    writeSectorsToFile(allSectors, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
    return 0;
}

