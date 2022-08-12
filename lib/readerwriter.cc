#include "globals.h"
#include "flags.h"
#include "fluxmap.h"
#include "readerwriter.h"
#include "protocol.h"
#include "usb/usb.h"
#include "encoders/encoders.h"
#include "decoders/decoders.h"
#include "fluxsource/fluxsource.h"
#include "fluxsink/fluxsink.h"
#include "imagereader/imagereader.h"
#include "imagewriter/imagewriter.h"
#include "fmt/format.h"
#include "sector.h"
#include "image.h"
#include "logger.h"
#include "mapper.h"
#include "utils.h"
#include "lib/config.pb.h"
#include "proto.h"
#include <optional>

enum ReadResult
{
    GOOD_READ,
    BAD_AND_CAN_RETRY,
    BAD_AND_CAN_NOT_RETRY
};

enum BadSectorsState
{
	HAS_NO_BAD_SECTORS,
	HAS_BAD_SECTORS
};

/* In order to allow rereads in file-based flux sources, we need to persist the
 * FluxSourceIterator (as that's where the state for which read to return is
 * held). This class handles that. */

class FluxSourceIteratorHolder
{
public:
    FluxSourceIteratorHolder(FluxSource& fluxSource): _fluxSource(fluxSource) {}

    FluxSourceIterator& getIterator(unsigned physicalCylinder, unsigned head)
    {
        auto& it = _cache[std::make_pair(physicalCylinder, head)];
        if (!it)
            it = _fluxSource.readFlux(physicalCylinder, head);
        return *it;
    }

private:
    FluxSource& _fluxSource;
    std::map<std::pair<unsigned, unsigned>, std::unique_ptr<FluxSourceIterator>>
        _cache;
};

/* Given a set of sectors, deduplicates them sensibly (e.g. if there is a good
 * and bad version of the same sector, the bad version is dropped). */

static std::set<std::shared_ptr<const Sector>> collectSectors(
    std::set<std::shared_ptr<const Sector>>& track_sectors,
    bool collapse_conflicts = true)
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
                    if (!collapse_conflicts)
                    {
                        auto s = std::make_shared<Sector>(*right);
                        s->status = Sector::CONFLICT;
                        sector_set.insert(s);
                    }
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

BadSectorsState combineRecordAndSectors(
    TrackFlux& trackFlux, AbstractDecoder& decoder, const Location& location)
{
    std::set<std::shared_ptr<const Sector>> track_sectors;

    for (auto& trackdataflux : trackFlux.trackDatas)
        track_sectors.insert(
            trackdataflux->sectors.begin(), trackdataflux->sectors.end());

    for (unsigned logical_sector : decoder.requiredSectors(trackFlux.location))
    {
        auto sector = std::make_shared<Sector>(location);
        sector->logicalSector = logical_sector;
        sector->status = Sector::MISSING;
        track_sectors.insert(sector);
    }

    trackFlux.sectors = collectSectors(track_sectors);
    if (trackFlux.sectors.empty())
        return HAS_BAD_SECTORS;
    for (const auto& sector : trackFlux.sectors)
        if (sector->status != Sector::OK)
            return HAS_BAD_SECTORS;

    return HAS_NO_BAD_SECTORS;
}

ReadResult readGroup(FluxSourceIteratorHolder& fluxSourceIteratorHolder,
    const Location& location,
    TrackFlux& trackFlux,
    AbstractDecoder& decoder)
{
    ReadResult result = BAD_AND_CAN_NOT_RETRY;

    for (unsigned offset = 0; offset < location.groupSize;
         offset += config.drive().head_width())
    {
        auto& fluxSourceIterator = fluxSourceIteratorHolder.getIterator(
            location.physicalTrack + offset, location.head);
        if (!fluxSourceIterator.hasNext())
            continue;

        Logger() << BeginReadOperationLogMessage{
            location.physicalTrack + offset, location.head};
        std::shared_ptr<const Fluxmap> fluxmap = fluxSourceIterator.next();
        // ->rescale(
        //     1.0 / config.flux_source().rescale());
        Logger() << EndReadOperationLogMessage()
                 << fmt::format("{0:.0} ms in {1} bytes",
                        fluxmap->duration() / 1e6,
                        fluxmap->bytes());

        auto trackdataflux = decoder.decodeToSectors(fluxmap, location);
        trackFlux.trackDatas.push_back(trackdataflux);
        if (combineRecordAndSectors(trackFlux, decoder, location) == HAS_NO_BAD_SECTORS)
		{
			result = GOOD_READ;
			if (config.decoder().skip_unnecessary_tracks())
				return result;
		}
        else if (fluxSourceIterator.hasNext())
            result = BAD_AND_CAN_RETRY;
    }

    return result;
}

void writeTracks(FluxSink& fluxSink,
    std::function<std::unique_ptr<const Fluxmap>(const Location& location)>
        producer,
    std::function<bool(const Location& location)> verifier)
{
    Logger() << fmt::format("Writing to: {}", (std::string)fluxSink);

    for (const auto& location : Mapper::computeLocations())
    {
        testForEmergencyStop();

        int retriesRemaining = config.decoder().retries();
        for (;;)
        {
            for (int offset = 0; offset < location.groupSize;
                 offset += config.drive().head_width())
            {
                unsigned physicalTrack = location.physicalTrack + offset;

                Logger() << BeginWriteOperationLogMessage{
                    physicalTrack, location.head};

                if (offset == config.drive().group_offset())
                {
                    auto fluxmap = producer(location);
                    if (!fluxmap)
                        goto erase;

                    fluxSink.writeFlux(physicalTrack, location.head, *fluxmap);
                    Logger() << fmt::format("writing {0} ms in {1} bytes",
                        int(fluxmap->duration() / 1e6),
                        fluxmap->bytes());
                }
                else
                {
                erase:
                    /* Erase this track rather than writing. */

                    Fluxmap blank;
                    fluxSink.writeFlux(physicalTrack, location.head, blank);
                    Logger() << "erased";
                }

                Logger() << EndWriteOperationLogMessage();
            }

            if (verifier(location))
                break;

            if (retriesRemaining == 0)
                Error() << "fatal error on write";

            Logger() << fmt::format(
                "retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }
    }
}

static bool dontVerify(const Location&)
{
    return true;
}

void writeTracks(
    FluxSink& fluxSink, AbstractEncoder& encoder, const Image& image)
{
    writeTracks(
        fluxSink,
        [&](const Location& location)
        {
            auto sectors = encoder.collectSectors(location, image);
            return encoder.encode(location, sectors, image);
        },
        dontVerify);
}

void writeTracksAndVerify(FluxSink& fluxSink,
    AbstractEncoder& encoder,
    FluxSource& fluxSource,
    AbstractDecoder& decoder,
    const Image& image)
{
    writeTracks(
        fluxSink,
        [&](const Location& location)
        {
            auto sectors = encoder.collectSectors(location, image);
            return encoder.encode(location, sectors, image);
        },
        [&](const Location& location)
        {
            auto trackFlux = std::make_shared<TrackFlux>();
            trackFlux->location = location;
            FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
            auto result = readGroup(
                fluxSourceIteratorHolder, location, *trackFlux, decoder);
            Logger() << TrackReadLogMessage{trackFlux};

            if (result != GOOD_READ)
            {
                Logger() << "bad read";
                return false;
            }

            auto wantedSectors = encoder.collectSectors(location, image);
            std::sort(wantedSectors.begin(),
                wantedSectors.end(),
                sectorPointerSortPredicate);

            std::vector<std::shared_ptr<const Sector>> gotSectors(
                trackFlux->sectors.begin(), trackFlux->sectors.end());
            std::sort(gotSectors.begin(),
                gotSectors.end(),
                sectorPointerSortPredicate);

            if (!std::equal(gotSectors.begin(),
                    gotSectors.end(),
                    wantedSectors.begin(),
                    wantedSectors.end(),
                    sectorPointerEqualsPredicate))
            {
                Logger() << "good read but the data doesn't match";
                return false;
            }
            return true;
        });
}

void writeDiskCommand(std::shared_ptr<const Image> image,
    AbstractEncoder& encoder,
    FluxSink& fluxSink,
    AbstractDecoder* decoder,
    FluxSource* fluxSource)
{
    if (config.has_sector_mapping())
        image = std::move(Mapper::remapSectorsLogicalToPhysical(
            *image, config.sector_mapping()));

    if (fluxSource && decoder)
        writeTracksAndVerify(fluxSink, encoder, *fluxSource, *decoder, *image);
    else
        writeTracks(fluxSink, encoder, *image);
}

void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink)
{
    writeTracks(
        fluxSink,
        [&](const Location& location)
        {
            return fluxSource.readFlux(location.physicalTrack, location.head)
                ->next();
        },
        dontVerify);
}

std::shared_ptr<const DiskFlux> readDiskCommand(
    FluxSource& fluxSource, AbstractDecoder& decoder)
{
    std::unique_ptr<FluxSink> outputFluxSink;
    if (config.decoder().has_copy_flux_to())
        outputFluxSink = FluxSink::create(config.decoder().copy_flux_to());

    auto diskflux = std::make_shared<DiskFlux>();
    bool failures = false;

    FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
    for (const auto& location : Mapper::computeLocations())
    {
        testForEmergencyStop();

        auto trackFlux = std::make_shared<TrackFlux>();
        trackFlux->location = location;
        diskflux->tracks.push_back(trackFlux);

        int retriesRemaining = config.decoder().retries();
        for (;;)
        {
            auto result = readGroup(
                fluxSourceIteratorHolder, location, *trackFlux, decoder);
            if (result == GOOD_READ)
                break;
            if (result == BAD_AND_CAN_NOT_RETRY)
            {
                failures = true;
                Logger() << fmt::format("no more data; giving up");
                break;
            }

            if (retriesRemaining == 0)
            {
                failures = true;
                Logger() << fmt::format("giving up");
                break;
            }

            Logger() << fmt::format(
                "retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }

        if (outputFluxSink)
        {
            for (const auto& data : trackFlux->trackDatas)
                outputFluxSink->writeFlux(
                    location.physicalTrack, location.head, *data->fluxmap);
        }

        if (config.decoder().dump_records())
        {
            std::vector<std::shared_ptr<const Record>> sorted_records;

            for (const auto& data : trackFlux->trackDatas)
                sorted_records.insert(sorted_records.end(),
                    data->records.begin(),
                    data->records.end());

            std::sort(sorted_records.begin(),
                sorted_records.end(),
                [](const auto& o1, const auto& o2)
                {
                    return o1->startTime < o2->startTime;
                });

            std::cout << "\nRaw (undecoded) records follow:\n\n";
            for (const auto& record : sorted_records)
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
            auto collected_sectors = collectSectors(trackFlux->sectors, false);
            std::vector<std::shared_ptr<const Sector>> sorted_sectors(
                collected_sectors.begin(), collected_sectors.end());
            std::sort(sorted_sectors.begin(),
                sorted_sectors.end(),
                [](const auto& o1, const auto& o2)
                {
                    return *o1 < *o2;
                });

            std::cout << "\nDecoded sectors follow:\n\n";
            for (const auto& sector : sorted_sectors)
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

        /* track can't be modified below this point. */
        Logger() << TrackReadLogMessage{trackFlux};
    }

    if (failures)
        Logger() << "Warning: some sectors could not be decoded.";

    std::set<std::shared_ptr<const Sector>> all_sectors;
    for (auto& track : diskflux->tracks)
        for (auto& sector : track->sectors)
            all_sectors.insert(sector);
    all_sectors = collectSectors(all_sectors);
    diskflux->image = std::make_shared<Image>(all_sectors);

    if (config.has_sector_mapping())
        diskflux->image = std::move(Mapper::remapSectorsPhysicalToLogical(
            *diskflux->image, config.sector_mapping()));

    /* diskflux can't be modified below this point. */
    Logger() << DiskReadLogMessage{diskflux};
    return diskflux;
}

void readDiskCommand(
    FluxSource& fluxsource, AbstractDecoder& decoder, ImageWriter& writer,FluxSink* solvedFlux)
{
    auto diskflux = readDiskCommand(fluxsource, decoder);

    writer.printMap(*diskflux->image);
    if (config.decoder().has_write_csv_to())
        writer.writeCsv(*diskflux->image, config.decoder().write_csv_to());
    writer.writeImage(*diskflux->image);
    if (config.has_solved_flux()) {
        std::unique_ptr<AbstractEncoder> solvedEncoder = AbstractEncoder::createFromImage(*config.mutable_encoder(),
    config.decoder(), *diskflux->image);
        writeTracks(*solvedFlux, *solvedEncoder, *diskflux->image);
    }
}

void rawReadDiskCommand(FluxSource& fluxsource, FluxSink& fluxsink)
{
    for (unsigned track : iterate(config.tracks()))
    {
        for (unsigned head : iterate(config.heads()))
        {
            testForEmergencyStop();
            auto fluxSourceIterator = fluxsource.readFlux(track, head);

            Logger() << BeginReadOperationLogMessage{track, head};
            auto fluxmap = fluxSourceIterator->next();
            Logger() << EndReadOperationLogMessage()
                     << fmt::format("{0:.0} ms in {1} bytes",
                            fluxmap->duration() / 1e6,
                            fluxmap->bytes());

            fluxsink.writeFlux(track, head, *fluxmap);
        }
    }
}

void fillBitmapTo(std::vector<bool>& bitmap,
    unsigned& cursor,
    unsigned terminateAt,
    const std::vector<bool>& pattern)
{
    while (cursor < terminateAt)
    {
        for (bool b : pattern)
        {
            if (cursor < bitmap.size())
                bitmap[cursor++] = b;
        }
    }
}
