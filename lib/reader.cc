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
#include "flux.h"
#include "image.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "proto.h"
#include "lib/decoders/decoders.pb.h"
#include "lib/data.pb.h"
#include <iostream>
#include <fstream>

static std::unique_ptr<FluxSink> outputFluxSink;

static std::shared_ptr<Fluxmap> readFluxmap(FluxSource& fluxsource, unsigned cylinder, unsigned head)
{
	std::cout << fmt::format("{0:>3}.{1}: ", cylinder, head) << std::flush;
	std::shared_ptr<Fluxmap> fluxmap = fluxsource.readFlux(cylinder, head);
	std::cout << fmt::format(
		"{0} ms in {1} bytes\n",
            fluxmap->duration()/1e6,
            fluxmap->bytes());
	if (outputFluxSink)
		outputFluxSink->writeFlux(cylinder, head, *fluxmap);
	return fluxmap;
}

static bool conflictable(Sector::Status status)
{
	return (status == Sector::OK) || (status == Sector::CONFLICT);
}

static std::set<std::shared_ptr<Sector>> collect_sectors(std::set<std::shared_ptr<Sector>>& track_sectors)
{
	typedef std::tuple<unsigned, unsigned, unsigned> key_t;
	std::map<key_t, std::shared_ptr<Sector>> sectors;

	for (auto& replacement : track_sectors)
	{
		key_t sectorid = {replacement->logicalTrack, replacement->logicalSide, replacement->logicalSector};
		auto replacing = sectors[sectorid];
		if (replacing && conflictable(replacing->status) && conflictable(replacement->status))
		{
			if (replacement->data != replacing->data)
			{
				std::cout << fmt::format(
						"\n       multiple conflicting copies of sector {} seen; ",
						std::get<0>(sectorid), std::get<1>(sectorid), std::get<2>(sectorid));
				replacing->status = replacement->status = Sector::CONFLICT;
			}
		}
		if (!replacing || ((replacing->status != Sector::OK) && (replacement->status == Sector::OK)))
			sectors[sectorid] = replacement;
	}

	std::set<std::shared_ptr<Sector>> sector_set;
	for (auto& i : sectors)
		sector_set.insert(i.second);
	return sector_set;
}

void readDiskCommand(FluxSource& fluxsource, AbstractDecoder& decoder, ImageWriter& writer)
{
	if (config.decoder().has_copy_flux_to())
		outputFluxSink = FluxSink::create(config.decoder().copy_flux_to());

	auto diskflux = std::make_unique<DiskFlux>();
	bool failures = false;
	for (int cylinder : iterate(config.cylinders()))
	{
		for (int head : iterate(config.heads()))
		{
			auto track = std::make_unique<TrackFlux>();
			std::set<std::shared_ptr<Sector>> track_sectors;
			std::set<std::shared_ptr<Record>> track_records;

			for (int retry = config.decoder().retries(); retry >= 0; retry--)
			{
				auto fluxmap = readFluxmap(fluxsource, cylinder, head);
				{
					auto trackdata = decoder.decodeToSectors(fluxmap, cylinder, head);

					std::cout << "       ";
						std::cout << fmt::format("{} records, {} sectors; ",
							trackdata->records.size(),
							trackdata->sectors.size());
					if (trackdata->sectors.size() > 0)
					{
						nanoseconds_t clock = (*trackdata->sectors.begin())->clock;
						std::cout << fmt::format("{:.2f}us clock ({:.0f}kHz); ",
							clock / 1000.0, 1000000.0 / clock);
					}

					track_sectors.insert(trackdata->sectors.begin(), trackdata->sectors.end());
					track_records.insert(trackdata->records.begin(), trackdata->records.end());
					track->trackDatas.push_back(std::move(trackdata));
				}
				auto collected_sectors = collect_sectors(track_sectors);
				std::cout << fmt::format("{} distinct sectors; ", collected_sectors.size());

				bool hasBadSectors = false;
				std::set<unsigned> required_sectors = decoder.requiredSectors(cylinder, head);
				for (const auto& sector : collected_sectors)
				{
					required_sectors.erase(sector->logicalSector);

					if (sector->status != Sector::OK)
					{
						std::cout << std::endl
								<< "       Failed to read sector " << sector->logicalSector
								<< " (" << Sector::statusToString(sector->status) << "); ";
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
								record->startTime / 1000.0,
								record->clock / 1000.0);
					hexdump(std::cout, record->rawData);
					std::cout << std::endl;
				}
			}

			if (config.decoder().dump_sectors())
			{
				std::cout << "\nDecoded sectors follow:\n\n";
				for (const auto& sector : track_sectors)
				{
					std::cout << fmt::format("{}.{:02}.{:02}: I+{:.2f}us with {:.2f}us clock: status {}\n",
								sector->logicalTrack,
								sector->logicalSide,
								sector->logicalSector,
								sector->headerStartTime / 1000.0,
								sector->clock / 1000.0,
								Sector::statusToString(sector->status));
					hexdump(std::cout, sector->data);
					std::cout << std::endl;
				}
			}

			int size = 0;
			std::set<std::pair<int, int>> track_ids;
			for (const auto& sector : track_sectors)
			{
				track_ids.insert(std::make_pair(sector->logicalTrack, sector->logicalSide));
				size += sector->data.size();
			}

			if (!track_ids.empty())
			{
				std::cout << "logical track ";
				for (const auto& i : track_ids)
					std::cout << fmt::format("{}.{}; ", i.first, i.second);
			}

			std::cout << size << " bytes decoded." << std::endl;
			track->sectors = collect_sectors(track_sectors);
			diskflux->tracks.push_back(std::move(track));
		}
    }

	std::set<std::shared_ptr<Sector>> all_sectors;
	for (auto& track : diskflux->tracks)
		for (auto& sector : track->sectors)
			all_sectors.insert(sector);
	all_sectors = collect_sectors(all_sectors);
	diskflux->image.reset(new Image(all_sectors));

	writer.printMap(*diskflux->image);
	if (config.decoder().has_write_csv_to())
		writer.writeCsv(*diskflux->image, config.decoder().write_csv_to());
	writer.writeImage(*diskflux->image);

	if (failures)
		std::cerr << "Warning: some sectors could not be decoded." << std::endl;
}

void rawReadDiskCommand(FluxSource& fluxsource, FluxSink& fluxsink)
{
	for (int cylinder : iterate(config.cylinders()))
	{
		for (int head : iterate(config.heads()))
		{
			auto fluxmap = readFluxmap(fluxsource, cylinder, head);
			fluxsink.writeFlux(cylinder, head, *fluxmap);
		}
    }
}
