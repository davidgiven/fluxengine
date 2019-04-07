#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "fluxreader.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "dataspec.h"
#include "decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "image.h"
#include "bytes.h"
#include "rawbits.h"
#include "fmt/format.h"

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":t=0-79:s=0-1:d=0");

static StringFlag destination(
    { "--write-flux", "-f" },
    "write the raw magnetic flux to this file",
    "");

static SettableFlag justRead(
	{ "--just-read" },
	"just read the disk and do no further processing");

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed records.");

static IntFlag retries(
	{ "--retries" },
	"How many times to retry each track in the event of a read failure.",
	5);

static SettableFlag highDensityFlag(
	{ "--high-density", "-H" },
	"set the drive to high density mode");

static sqlite3* outdb;

void setReaderDefaultSource(const std::string& source)
{
    ::source.set(source);
}

void setReaderRevolutions(int revolutions)
{
	setHardwareFluxReaderRevolutions(revolutions);
}

std::unique_ptr<Fluxmap> Track::read()
{
	std::cout << fmt::format("{0:>3}.{1}: ", track, side) << std::flush;
	std::unique_ptr<Fluxmap> fluxmap = _fluxReader->readFlux(track, side);
	std::cout << fmt::format(
		"{0} ms in {1} bytes\n",
            int(fluxmap->duration()/1e6),
            fluxmap->bytes());
	if (outdb)
		sqlWriteFlux(outdb, track, side, *fluxmap);
	return fluxmap;
}

void Track::recalibrate()
{
	_fluxReader->recalibrate();
}

bool Track::retryable()
{
	return _fluxReader->retryable();
}

std::vector<std::unique_ptr<Track>> readTracks()
{
    const DataSpec& dataSpec = source.value;

    std::cout << "Reading from: " << dataSpec << std::endl;

	setHardwareFluxReaderDensity(highDensityFlag);

	if (!destination.value.empty())
	{
		outdb = sqlOpen(destination, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		std::cout << "Writing a copy of the flux to " << destination.value << std::endl;
		sqlPrepareFlux(outdb);
		sqlStmt(outdb, "BEGIN;");
        sqlWriteIntProperty(outdb, "version", FLUX_VERSION_CURRENT);
		atexit([]()
			{
				sqlStmt(outdb, "COMMIT;");
				sqlClose(outdb);
			}
		);
	}

	std::shared_ptr<FluxReader> fluxreader = FluxReader::create(dataSpec);

	std::vector<std::unique_ptr<Track>> tracks;
    for (const auto& location : dataSpec.locations)
		tracks.push_back(
			std::unique_ptr<Track>(new Track(fluxreader, location.track, location.side)));

	if (justRead)
	{
		for (auto& track : tracks)
			track->read();
		
		std::cout << "--just-read specified, halting now" << std::endl;
		exit(0);
	}

	return tracks;
}

static bool conflictable(int status)
{
	return (status == Sector::OK) || (status == Sector::CONFLICT);
}

static void replace_sector(std::unique_ptr<Sector>& replacing, std::unique_ptr<Sector>& replacement)
{
	if (replacing && conflictable(replacing->status) && conflictable(replacement->status))
	{
		if (replacement->data != replacing->data)
		{
			std::cout << std::endl
						<< "       multiple conflicting copies of sector " << replacing->sector
						<< " seen; ";
			replacing->status = Sector::CONFLICT;
			return;
		}
	}
	if (!replacing || (replacing->status != Sector::OK))
		replacing = std::move(replacement);
}

void readDiskCommand(AbstractDecoder& decoder, const std::string& outputFilename)
{
	bool failures = false;
	SectorSet allSectors;
    for (const auto& track : readTracks())
	{
		std::map<int, std::unique_ptr<Sector>> readSectors;
		for (int retry = ::retries; retry >= 0; retry--)
		{
			std::unique_ptr<Fluxmap> fluxmap = track->read();

			nanoseconds_t clockPeriod = decoder.guessClock(*fluxmap);
			if (clockPeriod == 0)
			{
				std::cout << "       no clock detected; giving up" << std::endl;
				continue;
			}
			std::cout << fmt::format("       {:.2f} us clock; ", (double)clockPeriod/1000.0) << std::flush;

			const auto& bitmap = fluxmap->decodeToBits(clockPeriod);
			std::cout << fmt::format("{} bytes encoded; ", bitmap.size()/8) << std::flush;

			auto rawrecords = decoder.extractRecords(bitmap);
			std::cout << fmt::format("{} records", rawrecords.size()) << std::endl;

			auto sectors = decoder.decodeToSectors(rawrecords, track->track);
			std::cout << "       " << sectors.size() << " sectors; ";

			for (auto& sector : sectors)
			{
                auto& replacing = readSectors[sector->sector];
				replace_sector(replacing, sector);
			}

			bool hasBadSectors = false;
			for (const auto& i : readSectors)
			{
                const auto& sector = i.second;
				if (sector->status != Sector::OK)
				{
					std::cout << std::endl
							  << "       Failed to read sector " << sector->sector
                              << " (" << Sector::statusToString((Sector::Status)sector->status) << "); ";
					hasBadSectors = true;
				}
			}

			if (hasBadSectors)
				failures = false;

			if (dumpRecords && (!hasBadSectors || (retry == 0) || !track->retryable()))
			{
				std::cout << "\nRaw (undecoded) records follow:\n\n";
				for (auto& record : rawrecords)
				{
					std::cout << fmt::format("I+{:.3f}ms", (double)(record->position*clockPeriod)/1e6)
					          << std::endl;
					hexdump(std::cout, toBytes(record->data));
					std::cout << std::endl;
				}
			}

			std::cout << std::endl
                      << "       ";

			if (!hasBadSectors)
				break;

			if (!track->retryable())
				break;
            if (retry == 0)
                std::cout << "giving up" << std::endl
                          << "       ";
            else
            {
				std::cout << retry << " retries remaining" << std::endl;
                track->recalibrate();
            }
		}

        int size = 0;
		bool printedTrack = false;
        for (auto& i : readSectors)
        {
			auto& sector = i.second;
			if (sector)
			{
				if (!printedTrack)
				{
					std::cout << fmt::format("logical track {}.{}; ", sector->track, sector->side);
					printedTrack = true;
				}

				size += sector->data.size();

				auto& replacing = allSectors.get(sector->track, sector->side, sector->sector);
				replace_sector(replacing, sector);
			}
        }
        std::cout << size << " bytes decoded." << std::endl;
    }

	Geometry geometry = guessGeometry(allSectors);
    writeSectorsToFile(allSectors, geometry, outputFilename);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
}
