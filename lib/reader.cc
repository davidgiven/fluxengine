#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "reader.h"
#include "fluxmap.h"
#include "sql.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "sectorset.h"
#include "record.h"
#include "bytes.h"
#include "decoders/rawbits.h"
#include "track.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "proto.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/data.pb.h"
#include <iostream>
#include <fstream>

static std::unique_ptr<FluxSink> outputFluxSink;

static void readFluxmap(FluxSource& fluxsource, FluxTrackProto& track)
{
	int c = track.physical_cylinder();
	int h = track.physical_head();
	std::cout << fmt::format("{0:>3}.{1}: ", c, h) << std::flush;
	std::unique_ptr<Fluxmap> fluxmap = fluxsource.readFlux(c, h);
	track.set_data(fluxmap->rawBytes());
	std::cout << fmt::format(
		"{0} ms in {1} bytes\n",
            fluxmap->duration()/1e6,
            fluxmap->bytes());
	if (outputFluxSink)
		outputFluxSink->writeFlux(c, h, *fluxmap);
}

static bool conflictable(SectorStatus status)
{
	return (status == SectorStatus::OK) || (status == SectorStatus::CONFLICT);
}

static std::map<int, SectorProto*> collect_sectors(std::vector<SectorProto*>& track_sectors)
{
	std::map<int, SectorProto*> sectors;

	for (auto* replacement : track_sectors)
	{
		int sectorid = replacement->logical_sector();
		SectorProto* replacing = sectors[sectorid];
		if (replacing && conflictable(replacing->status()) && conflictable(replacement->status()))
		{
			if (replacement->data() != replacing->data())
			{
				std::cout << std::endl
							<< "       multiple conflicting copies of sector " << replacing->logical_sector()
							<< " seen; ";
				replacing->set_status(SectorStatus::CONFLICT);
			}
		}
		if (!replacing || ((replacing->status() != SectorStatus::OK) && (replacement->status() == SectorStatus::OK)))
			sectors[sectorid] = replacement;
	}

	return sectors;
}

void readDiskCommand(FluxSource& fluxsource, AbstractDecoder& decoder, ImageWriter& writer)
{
	if (config.decoder().has_copy_flux_to())
		outputFluxSink = FluxSink::create(config.decoder().copy_flux_to());

	bool failures = false;
	SectorSet allSectors;
	FluxProto fluxProto;
	for (int cylinder : iterate(config.cylinders()))
	{
		for (int head : iterate(config.heads()))
		{
			std::vector<SectorProto*> track_sectors;
			std::vector<const FluxRecordProto*> track_records;

			for (int retry = config.decoder().retries(); retry >= 0; retry--)
			{
				FluxTrackProto* track = fluxProto.add_track();
				track->set_physical_cylinder(cylinder);
				track->set_physical_head(head);

				readFluxmap(fluxsource, *track);
				decoder.decodeToSectors(*track);

				std::cout << "       ";
					std::cout << fmt::format("{} records, {} sectors; ",
						track->record_size(),
						track->sector_size());
				if (track->sector_size() > 0)
				{
					const auto clock = track->sector(0).clock();
					std::cout << fmt::format("{:.2f}us clock ({:.0f}kHz); ",
						clock / 1000.0, 1000000.0 / clock);
				}

				for (auto& record : track->record())
					track_records.push_back(&record);
				for (auto& sector : *track->mutable_sector())
					track_sectors.push_back(&sector);
				auto collected_sectors = collect_sectors(track_sectors);

				bool hasBadSectors = false;
				std::set<unsigned> required_sectors = decoder.requiredSectors(*track);
				for (const auto& i : collected_sectors)
				{
					const auto& sector = i.second;
					required_sectors.erase(sector->logical_sector());

					if (sector->status() != SectorStatus::OK)
					{
						std::cout << std::endl
								<< "       Failed to read sector " << i.first
								<< " (" << SectorStatus_Name(sector->status()) << "); ";
						hasBadSectors = true;
					}
				}
				for (unsigned logical_sector : required_sectors)
				{
					std::cout << "\n"
							  << "       Required sector " << logical_sector << " missing; ";
					hasBadSectors = true;
				}

				if (hasBadSectors)
					failures = false;

				std::cout << std::endl
						<< "       ";

				if (!hasBadSectors)
					break;

				if (!fluxsource.retryable())
					break;
				if (retry == 0)
					std::cout << "giving up" << std::endl
							  << "       ";
				else
					std::cout << retry << " retries remaining" << std::endl;
			}

			if (config.decoder().dump_records())
			{
				std::cout << "\nRaw (undecoded) records follow:\n\n";
				for (const auto& record : track_records)
				{
					std::cout << fmt::format("I+{:.2f}us with {:.2f}us clock\n",
								record.record_starttime_ns() / 1000.0, record.clock() / 1000.0);
					hexdump(std::cout, record.data());
					std::cout << std::endl;
				}
			}

			if (config.decoder().dump_sectors())
			{
				std::cout << "\nDecoded sectors follow:\n\n";
				for (const auto& sector : track->sectors())
				{
					std::cout << fmt::format("{}.{:02}.{:02}: I+{:.2f}us with {:.2f}us clock: status {}\n",
								sector.logical_track(),
								sector.logical_side(),
								sector.logical_sector(),
								sector.position_ns() / 1000.0,
								sector.clock() / 1000.0,
								SectorStatus_Name(sector.status()));
					hexdump(std::cout, sector.data());
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
    }

	writer.printMap(allSectors);
	if (config.decoder().has_write_csv_to())
		writer.writeCsv(allSectors, config.decoder().write_csv_to());
	writer.writeImage(allSectors);

	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
}

void rawReadDiskCommand(FluxSource& fluxsource, FluxSink& fluxsink)
{
	for (int cylinder : iterate(config.cylinders()))
	{
		for (int head : iterate(config.heads()))
		{
			Track track(cylinder, head);
			track.fluxsource = &fluxsource;
			track.readFluxmap();

			fluxsink.writeFlux(cylinder, head, *(track.fluxmap));
		}
    }
}
