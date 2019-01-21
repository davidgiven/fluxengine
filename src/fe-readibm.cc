#include "globals.h"
#include "flags.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders.h"
#include "image.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include <fmt/format.h>

static StringFlag outputFilename(
    { "--output", "-o" },
    "The output image file to write to.",
    "ibm.img");

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed records.");

static IntFlag sectorIdBase(
	{ "--sector-id-base" },
	"Sector ID of the first sector.",
	1);

int main(int argc, const char* argv[])
{
	setReaderDefaultSource(":t=0-79:s=0-1");
    Flag::parseFlags(argc, argv);

	IbmRecordParser recordParser(sectorIdBase);
	MfmBitmapDecoder bitmapDecoder;

	bool failures = false;
	SectorSet allSectors;
    for (auto& track : readTracks())
    {
		int retries = 5;
		std::map<int, std::unique_ptr<Sector>> goodSectors;

	retry:
		std::unique_ptr<Fluxmap> fluxmap = track->read();
		nanoseconds_t clockPeriod = fluxmap->guessClock();
		std::cout << fmt::format("       {:.1f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

		/* For MFM, the bit clock is half the detected clock. */
		auto bitmap = fluxmap->decodeToBits(clockPeriod/2);
		std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

		auto records = bitmapDecoder.decodeBitsToRecords(bitmap);
		std::cout << records.size() << " records." << std::endl;

		auto sectors = recordParser.parseRecordsToSectors(records);
		std::cout << "       " << sectors.size() << " sectors; ";

		bool hasBadSectors = false;
		for (auto& sector : sectors)
		{
			bool sectorPending = goodSectors.find(sector->sector) == goodSectors.end();
			if (sectorPending)
			{
				if (sector->status != Sector::OK)
				{
					std::cout << std::endl
							<< "       Bad CRC on sector " << sector->sector << "; ";
					hasBadSectors = true;
				}

				if (((sector->status == Sector::OK) || (retries == 0)) && (sector->sector >= 0))
					goodSectors[sector->sector] = std::move(sector);
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
				goto retry;
			}
		}

		int size = 0;
		for (auto& i : goodSectors)
		{
			auto& sector = i.second;
			size += sector->data.size();
			allSectors[{sector->track, sector->side, sector->sector}] =
				std::move(sector);
		}
		std::cout << size << " bytes decoded." << std::endl;

		if (dumpRecords)
		{
			std::cout << "\nRaw records follow:\n\n";
			for (auto& record : records)
			{
				std::cout << fmt::format("I+{:.3f}ms", (double)(record->position*clockPeriod)/1e6)
						  << std::endl;
				hexdump(std::cout, record->data);
				std::cout << std::endl;
			}
		}
    }

	Geometry geometry = guessGeometry(allSectors);
    writeSectorsToFile(allSectors, geometry, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
    return 0;
}

