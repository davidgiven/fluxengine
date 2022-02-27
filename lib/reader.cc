#include "globals.h"
#include "flags.h"
#include "usb/usb.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "reader.h"
#include "fluxmap.h"
#include "decoders/decoders.h"
#include "sector.h"
#include "bytes.h"
#include "decoders/rawbits.h"
#include "flux.h"
#include "image.h"
#include "imagewriter/imagewriter.h"
#include "logger.h"
#include "fmt/format.h"
#include "proto.h"
#include "lib/decoders/decoders.pb.h"
#include <iostream>
#include <fstream>

static std::unique_ptr<FluxSink> outputFluxSink;

static std::shared_ptr<Fluxmap> readFluxmap(
    FluxSource& fluxsource, unsigned cylinder, unsigned head)
{
    Logger() << DiskContextLogMessage{cylinder, head}
             << BeginReadOperationLogMessage();
    std::shared_ptr<Fluxmap> fluxmap = fluxsource.readFlux(cylinder, head);
    fluxmap->rescale(1.0 / config.flux_source().rescale());
    Logger() << EndReadOperationLogMessage()
             << fmt::format("{0:.0} ms in {1} bytes",
                    fluxmap->duration() / 1e6,
                    fluxmap->bytes());
    return fluxmap;
}

static bool conflictable(Sector::Status status)
{
    return (status == Sector::OK) || (status == Sector::CONFLICT);
}

static std::set<std::shared_ptr<const Sector>> collect_sectors(
    std::set<std::shared_ptr<const Sector>>& track_sectors)
{
    typedef std::tuple<unsigned, unsigned, unsigned> key_t;
    std::multimap<key_t, std::shared_ptr<const Sector>> sectors;

    for (const auto& sector : track_sectors)
    {
        key_t sectorid = {
            sector->logicalTrack, sector->logicalSide, sector->logicalSector};
        sectors.insert({sectorid, sector});
    }

    std::set<std::shared_ptr<const Sector>> sector_set;
    auto it = sectors.begin();
    while (it != sectors.end())
    {
        auto ub = sectors.upper_bound(it->first);
        auto new_sector = std::accumulate(it,
            ub,
            it->second,
            [&](auto left, auto& rightit) -> std::shared_ptr<const Sector>
            {
                auto& right = rightit.second;
                if ((left->status == Sector::OK) &&
                    (right->status == Sector::OK) &&
                    (left->data != right->data))
                {
                    auto s = std::make_shared<Sector>(*left);
                    s->status = Sector::CONFLICT;
                    return s;
                }
                if (left->status == Sector::CONFLICT)
                    return left;
                if (right->status == Sector::CONFLICT)
                    return right;
                if (left->status == Sector::OK)
                    return left;
                if (right->status == Sector::OK)
                    return right;
                return left;
            });
        sector_set.insert(new_sector);
        it = ub;
    }
    return sector_set;
}

void readDiskCommand(
    FluxSource& fluxsource, AbstractDecoder& decoder, ImageWriter& writer)
{
    if (config.decoder().has_copy_flux_to())
        outputFluxSink = FluxSink::create(config.decoder().copy_flux_to());

    auto diskflux = std::make_unique<DiskFlux>();
    bool failures = false;
    for (int cylinder : iterate(config.cylinders()))
    {
        for (int head : iterate(config.heads()))
        {
            auto track = std::make_shared<TrackFlux>();
            std::set<std::shared_ptr<const Sector>> track_sectors;
            std::set<std::shared_ptr<const Record>> track_records;
            Fluxmap totalFlux;

            for (int retry = config.decoder().retries(); retry >= 0; retry--)
            {
                auto fluxmap = readFluxmap(fluxsource, cylinder, head);
                totalFlux.appendDesync().appendBytes(fluxmap->rawBytes());

                auto maps = fluxmap->split();

                std::shared_ptr<const TrackDataFlux> trackdataflux;
                bool hasBadSectors = false;

                for (auto& map : maps)
                {
                    {
                        auto fm = std::make_shared<Fluxmap>();
                        fm->appendBytes(map.rawBytes());

                        trackdataflux = decoder.decodeToSectors(fm, cylinder, head);
                        track->trackDatas.push_back(trackdataflux);
					}

                    track_sectors.insert(trackdataflux->sectors.begin(),
                        trackdataflux->sectors.end());
                    track_records.insert(trackdataflux->records.begin(),
                        trackdataflux->records.end());

                    std::set<unsigned> required_sectors =
                        decoder.requiredSectors(cylinder, head);
                    std::set<std::shared_ptr<const Sector>> result_sectors;
                    for (const auto& sector : collect_sectors(track_sectors))
                    {
                        result_sectors.insert(sector);
                        required_sectors.erase(sector->logicalSector);

                        if (sector->status != Sector::OK)
                            hasBadSectors = true;
                    }
                    for (unsigned logical_sector : required_sectors)
                    {
                        auto sector = std::make_shared<Sector>();
                        sector->logicalSector = logical_sector;
                        sector->status = Sector::MISSING;
                        result_sectors.insert(sector);

                        hasBadSectors = true;
                    }

                    Logger() << SingleReadLogMessage{trackdataflux, result_sectors};
                }
                if (hasBadSectors)
                    failures = false;

                if (!hasBadSectors)
                    break;

                if (!fluxsource.retryable())
                    break;
                if (retry == 0)
                    Logger() << fmt::format("giving up");
                else
                    Logger()
                        << fmt::format("retrying; {} retries remaining", retry);
            }

            if (outputFluxSink)
                outputFluxSink->writeFlux(cylinder, head, totalFlux);

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
                    std::cout << fmt::format(
                        "{}.{:02}.{:02}: I+{:.2f}us with {:.2f}us clock: "
                        "status {}\n",
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

            for (const auto& sector : collect_sectors(track_sectors))
                track->sectors.insert(sector);

            Logger() << TrackReadLogMessage{track};
            diskflux->tracks.push_back(track);
        }
    }

    std::set<std::shared_ptr<const Sector>> all_sectors;
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
