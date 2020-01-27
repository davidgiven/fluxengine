#include "globals.h"
#include "flags.h"
#include "usb.h"
#include "fluxsource/fluxsource.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "dataspec.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "visualiser.h"
#include "record.h"
#include "image.h"
#include "bytes.h"
#include "decoders/rawbits.h"
#include "track.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"

FlagGroup readerFlags { &hardwareFluxSourceFlags, &fluxmapReaderFlags, &visualiserFlags };

static DataSpecFlag source(
    { "--source", "-s" },
    "source for data",
    ":t=0-79:s=0-1:d=0");

static DataSpecFlag output(
	{ "--output", "-o" },
	"output image file to write to",
	"");

static StringFlag destination(
    { "--write-flux", "-f" },
    "write the raw magnetic flux to this file",
    "");

static StringFlag visualise(
	{ "--write-svg" },
	"write a visualisation of the disk to this file",
	"");

static SettableFlag justRead(
	{ "--just-read" },
	"just read the disk and do no further processing");

static SettableFlag dumpRecords(
	{ "--dump-records" },
	"Dump the parsed but undecoded records.");

static SettableFlag dumpSectors(
	{ "--dump-sectors" },
	"Dump the decoded sectors.");

static IntFlag retries(
	{ "--retries" },
	"How many times to retry each track in the event of a read failure.",
	5);

static SettableFlag highDensityFlag(
	{ "--high-density", "--hd" },
	"set the drive to high density mode");

static sqlite3* outdb;

void setReaderDefaultSource(const std::string& source)
{
    ::source.set(source);
}

void setReaderDefaultOutput(const std::string& output)
{
    ::output.set(output);
}

void setReaderRevolutions(int revolutions)
{
	setHardwareFluxSourceRevolutions(revolutions);
}

void Track::readFluxmap()
{
	std::cout << fmt::format("{0:>3}.{1}: ", physicalTrack, physicalSide) << std::flush;
	fluxmap = fluxsource->readFlux(physicalTrack, physicalSide);
	std::cout << fmt::format(
		"{0} ms in {1} bytes\n",
            int(fluxmap->duration()/1e6),
            fluxmap->bytes());
	if (outdb)
		sqlWriteFlux(outdb, physicalTrack, physicalSide, *fluxmap);
}

std::vector<std::unique_ptr<Track>> readTracks()
{
    const FluxSpec spec(source);

    std::cout << "Reading from: " << source << std::endl;

	setHardwareFluxSourceDensity(highDensityFlag);

	if (!destination.get().empty())
	{
		outdb = sqlOpen(destination, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE);
		std::cout << "Writing a copy of the flux to " << destination.get() << std::endl;
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

	std::shared_ptr<FluxSource> fluxSource = FluxSource::create(spec);

	std::vector<std::unique_ptr<Track>> tracks;
    for (const auto& location : spec.locations)
	{
		auto track = std::make_unique<Track>(location.track, location.side);
		track->fluxsource = fluxSource;
		tracks.push_back(std::move(track));
	}

	if (justRead)
	{
		for (auto& track : tracks)
			track->readFluxmap();
		
		std::cout << "--just-read specified, halting now" << std::endl;
		exit(0);
	}

	return tracks;
}

static bool conflictable(Sector::Status status)
{
	return (status == Sector::OK) || (status == Sector::CONFLICT);
}

static void replace_sector(std::unique_ptr<Sector>& replacing, Sector& replacement)
{
	if (replacing && conflictable(replacing->status) && conflictable(replacement.status))
	{
		if (replacement.data != replacing->data)
		{
			std::cout << std::endl
						<< "       multiple conflicting copies of sector " << replacing->logicalSector
						<< " seen; ";
			replacing->status = Sector::CONFLICT;
			return;
		}
	}
	if (!replacing || ((replacing->status != Sector::OK) && (replacement.status == Sector::OK)))
	{
		if (!replacing)
			replacing.reset(new Sector);
		*replacing = replacement;
	}
}

void readDiskCommand(AbstractDecoder& decoder)
{
	const ImageSpec outputSpec(output);
	ImageWriter::verifyImageSpec(outputSpec);
        
	bool failures = false;
	SectorSet allSectors;
	auto tracks = readTracks();
    for (const auto& track : tracks)
	{
		std::map<int, std::unique_ptr<Sector>> readSectors;
		for (int retry = ::retries; retry >= 0; retry--)
		{
			track->readFluxmap();
			decoder.decodeToSectors(*track);

			std::cout << "       ";
			if (!track->sectors.empty())
			{
				std::cout << fmt::format("{} records, {} sectors; ",
					track->rawrecords.size(),
					track->sectors.size());
				if (track->sectors.size() > 0)
					std::cout << fmt::format("{:.2f}us clock ({:.0f}kHz); ",
						track->sectors.begin()->clock / 1000.0,
                        1000000.0 / track->sectors.begin()->clock);

				for (auto& sector : track->sectors)
				{
					auto& replacing = readSectors[sector.logicalSector];
					replace_sector(replacing, sector);
				}

				bool hasBadSectors = false;
				for (const auto& i : readSectors)
				{
					const auto& sector = i.second;
					if (sector->status != Sector::OK)
					{
						std::cout << std::endl
								<< "       Failed to read sector " << sector->logicalSector
								<< " (" << Sector::statusToString((Sector::Status)sector->status) << "); ";
						hasBadSectors = true;
					}
				}

				if (hasBadSectors)
					failures = false;

				std::cout << std::endl
						<< "       ";

				if (!hasBadSectors)
					break;
			}

			if (!track->fluxsource->retryable())
				break;
            if (retry == 0)
                std::cout << "giving up" << std::endl
                          << "       ";
            else
				std::cout << retry << " retries remaining" << std::endl;
		}

		if (dumpRecords)
		{
			std::cout << "\nRaw (undecoded) records follow:\n\n";
			for (auto& record : track->rawrecords)
			{
				std::cout << fmt::format("I+{:.2f}us with {:.2f}us clock\n",
                            record.position.ns() / 1000.0, record.clock / 1000.0);
				hexdump(std::cout, record.data);
				std::cout << std::endl;
			}
		}

        if (dumpSectors)
        {
            std::cout << "\nDecoded sectors follow:\n\n";
            for (auto& i : readSectors)
            {
                auto& sector = i.second;
				std::cout << fmt::format("{}.{:02}.{:02}: I+{:.2f}us with {:.2f}us clock\n",
                            sector->logicalTrack, sector->logicalSide, sector->logicalSector,
                            sector->position.ns() / 1000.0, sector->clock / 1000.0);
				hexdump(std::cout, sector->data);
				std::cout << std::endl;
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
					std::cout << fmt::format("logical track {}.{}; ", sector->logicalTrack, sector->logicalSide);
					printedTrack = true;
				}

				size += sector->data.size();

				std::unique_ptr<Sector>& replacing = allSectors.get(sector->logicalTrack, sector->logicalSide, sector->logicalSector);
				replace_sector(replacing, *sector);
			}
        }
        std::cout << size << " bytes decoded." << std::endl;
    }

	if (!visualise.get().empty())
		visualiseSectorsToFile(allSectors, visualise.get());

    writeSectorsToFile(allSectors, outputSpec);
	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
}
