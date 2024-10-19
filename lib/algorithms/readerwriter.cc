#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/config/flags.h"
#include "lib/data/fluxmap.h"
#include "lib/algorithms/readerwriter.h"
#include "protocol.h"
#include "lib/usb/usb.h"
#include "lib/encoders/encoders.h"
#include "lib/decoders/decoders.h"
#include "lib/fluxsource/fluxsource.h"
#include "lib/fluxsink/fluxsink.h"
#include "lib/imagereader/imagereader.h"
#include "lib/imagewriter/imagewriter.h"
#include "lib/data/sector.h"
#include "lib/data/image.h"
#include "lib/core/logger.h"
#include "lib/data/layout.h"
#include "lib/core/utils.h"
#include "lib/config/config.pb.h"
#include "lib/config/proto.h"
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

/* Log renderers. */

/* Start measuring the rotational speed */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginSpeedOperationLogMessage> m)
{
    r.newline().add("Measuring rotational speed...").newline();
}

/* Finish measuring the rotational speed */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndSpeedOperationLogMessage> m)
{
    r.newline()
        .add(fmt::format("Rotational period is {:.1f}ms ({:.1f}rpm)",
            m->rotationalPeriod / 1e6,
            60e9 / m->rotationalPeriod))
        .newline();
}

/* Indicates that we're starting a write operation. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginWriteOperationLogMessage> m)
{
    r.header(fmt::format("W{:2}.{}: ", m->track, m->head));
}

/* Indicates that we're finishing a write operation. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndWriteOperationLogMessage> m)
{
}

/* Indicates that we're starting a read operation. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginReadOperationLogMessage> m)
{
    r.header(fmt::format("R{:2}.{}: ", m->track, m->head));
}

/* Indicates that we're finishing a read operation. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndReadOperationLogMessage> m)
{
}

/* We've just read a track (we might reread it if there are errors)
 */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const TrackReadLogMessage> m)
{
    const auto& track = *m->track;

    std::set<std::shared_ptr<const Sector>> rawSectors;
    std::set<std::shared_ptr<const Record>> rawRecords;
    for (const auto& trackDataFlux : track.trackDatas)
    {
        rawSectors.insert(
            trackDataFlux->sectors.begin(), trackDataFlux->sectors.end());
        rawRecords.insert(
            trackDataFlux->records.begin(), trackDataFlux->records.end());
    }

    nanoseconds_t clock = 0;
    for (const auto& sector : rawSectors)
        clock += sector->clock;
    if (!rawSectors.empty())
        clock /= rawSectors.size();

    r.comma().add(fmt::format("{} raw records, {} raw sectors",
        rawRecords.size(),
        rawSectors.size()));
    if (clock != 0)
    {
        r.comma().add(fmt::format(
            "{:.2f}us clock ({:.0f}kHz)", clock / 1000.0, 1000000.0 / clock));
    }

    r.newline().add("sectors:");

    std::vector<std::shared_ptr<const Sector>> sectors(
        track.sectors.begin(), track.sectors.end());
    std::sort(sectors.begin(), sectors.end(), sectorPointerSortPredicate);

    for (const auto& sector : sectors)
        r.add(fmt::format("{}.{}.{}{}",
            sector->logicalTrack,
            sector->logicalSide,
            sector->logicalSector,
            Sector::statusToChar(sector->status)));

    int size = 0;
    std::set<std::pair<int, int>> track_ids;
    for (const auto& sector : m->track->sectors)
    {
        track_ids.insert(
            std::make_pair(sector->logicalTrack, sector->logicalSide));
        size += sector->data.size();
    }

    r.newline().add(fmt::format("{} bytes decoded\n", size));
}

/* We've just read a disk.
 */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const DiskReadLogMessage> m)
{
}

/* Large-scale operation start. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const BeginOperationLogMessage> m)
{
}

/* Large-scale operation end. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const EndOperationLogMessage> m)
{
}

/* Large-scale operation progress. */
void renderLogMessage(
    LogRenderer& r, std::shared_ptr<const OperationProgressLogMessage> m)
{
}

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

void measureDiskRotation()
{
    log(BeginSpeedOperationLogMessage());

    nanoseconds_t oneRevolution =
        globalConfig()->drive().rotational_period_ms() * 1e6;
    if (oneRevolution == 0)
    {
        usbSetDrive(globalConfig()->drive().drive(),
            globalConfig()->drive().high_density(),
            globalConfig()->drive().index_mode());

        log(BeginOperationLogMessage{"Measuring drive rotational speed"});
        int retries = 5;
        do
        {
            oneRevolution = usbGetRotationalPeriod(
                globalConfig()->drive().hard_sector_count());

            retries--;
        } while ((oneRevolution == 0) && (retries > 0));
        globalConfig().setTransient(
            "drive.rotational_period_ms", std::to_string(oneRevolution / 1e6));
        log(EndOperationLogMessage{});
    }

    if (!globalConfig()->drive().hard_sector_threshold_ns())
    {
        int count = globalConfig()->drive().hard_sector_count();
        globalConfig().setTransient("drive.hard_sector_threshold_ns",
            count ? std::to_string(
                        oneRevolution * 3 /
                        (4 * globalConfig()->drive().hard_sector_count()))
                  : "0");
    }

    if (oneRevolution == 0)
        error("Failed\nIs a disk in the drive?");

    log(EndSpeedOperationLogMessage{oneRevolution});
}

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

BadSectorsState combineRecordAndSectors(TrackFlux& trackFlux,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& trackLayout)
{
    std::set<std::shared_ptr<const Sector>> track_sectors;

    /* Add the sectors which were there. */

    for (auto& trackdataflux : trackFlux.trackDatas)
        track_sectors.insert(
            trackdataflux->sectors.begin(), trackdataflux->sectors.end());

    /* Add the sectors which should be there. */

    for (unsigned sectorId : trackLayout->naturalSectorOrder)
    {
        auto sector = std::make_shared<Sector>(LogicalLocation{
            trackLayout->logicalTrack, trackLayout->logicalSide, sectorId});

        sector->status = Sector::MISSING;
        track_sectors.insert(sector);
    }

    /* Deduplicate. */

    trackFlux.sectors = collectSectors(track_sectors);
    if (trackFlux.sectors.empty())
        return HAS_BAD_SECTORS;
    for (const auto& sector : trackFlux.sectors)
        if (sector->status != Sector::OK)
            return HAS_BAD_SECTORS;

    return HAS_NO_BAD_SECTORS;
}

static void adjustTrackOnError(FluxSource& fluxSource, int baseTrack)
{
    switch (globalConfig()->drive().error_behaviour())
    {
        case DriveProto::NOTHING:
            break;

        case DriveProto::RECALIBRATE:
            fluxSource.recalibrate();
            break;

        case DriveProto::JIGGLE:
            if (baseTrack > 0)
                fluxSource.seek(baseTrack - 1);
            else
                fluxSource.seek(baseTrack + 1);
            break;
    }
}

ReadResult readGroup(FluxSourceIteratorHolder& fluxSourceIteratorHolder,
    std::shared_ptr<const TrackInfo>& trackInfo,
    TrackFlux& trackFlux,
    Decoder& decoder)
{
    ReadResult result = BAD_AND_CAN_NOT_RETRY;

    for (unsigned offset = 0; offset < trackInfo->groupSize;
         offset += Layout::getHeadWidth())
    {
        log(BeginReadOperationLogMessage{
            trackInfo->physicalTrack + offset, trackInfo->physicalSide});

        auto& fluxSourceIterator = fluxSourceIteratorHolder.getIterator(
            trackInfo->physicalTrack + offset, trackInfo->physicalSide);
        if (!fluxSourceIterator.hasNext())
            continue;

        std::shared_ptr<const Fluxmap> fluxmap = fluxSourceIterator.next();
        // ->rescale(
        //     1.0 / globalConfig()->flux_source().rescale());
        log(EndReadOperationLogMessage());
        log("{0} ms in {1} bytes",
            (int)(fluxmap->duration() / 1e6),
            fluxmap->bytes());

        auto trackdataflux = decoder.decodeToSectors(fluxmap, trackInfo);
        trackFlux.trackDatas.push_back(trackdataflux);
        if (combineRecordAndSectors(trackFlux, decoder, trackInfo) ==
            HAS_NO_BAD_SECTORS)
        {
            result = GOOD_READ;
            if (globalConfig()->decoder().skip_unnecessary_tracks())
                return result;
        }
        else if (fluxSourceIterator.hasNext())
            result = BAD_AND_CAN_RETRY;
    }

    return result;
}

void writeTracks(FluxSink& fluxSink,
    std::function<std::unique_ptr<const Fluxmap>(
        std::shared_ptr<const TrackInfo>& trackInfo)> producer,
    std::function<bool(std::shared_ptr<const TrackInfo>& trackInfo)> verifier,
    std::vector<std::shared_ptr<const TrackInfo>>& trackInfos)
{
    log(BeginOperationLogMessage{"Encoding and writing to disk"});

    if (fluxSink.isHardware())
        measureDiskRotation();
    int index = 0;
    for (auto& trackInfo : trackInfos)
    {
        log(OperationProgressLogMessage{
            index * 100 / (unsigned)trackInfos.size()});
        index++;

        testForEmergencyStop();

        int retriesRemaining = globalConfig()->decoder().retries();
        for (;;)
        {
            for (int offset = 0; offset < trackInfo->groupSize;
                 offset += Layout::getHeadWidth())
            {
                unsigned physicalTrack = trackInfo->physicalTrack + offset;

                log(BeginWriteOperationLogMessage{
                    physicalTrack, trackInfo->physicalSide});

                if (offset == globalConfig()->drive().group_offset())
                {
                    auto fluxmap = producer(trackInfo);
                    if (!fluxmap)
                        goto erase;

                    fluxSink.writeFlux(
                        physicalTrack, trackInfo->physicalSide, *fluxmap);
                    log("writing {0} ms in {1} bytes",
                        int(fluxmap->duration() / 1e6),
                        fluxmap->bytes());
                }
                else
                {
                erase:
                    /* Erase this track rather than writing. */

                    Fluxmap blank;
                    fluxSink.writeFlux(
                        physicalTrack, trackInfo->physicalSide, blank);
                    log("erased");
                }

                log(EndWriteOperationLogMessage());
            }

            if (verifier(trackInfo))
                break;

            if (retriesRemaining == 0)
                error("fatal error on write");

            log("retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }
    }

    log(EndOperationLogMessage{"Write complete"});
}

void writeTracks(FluxSink& fluxSink,
    Encoder& encoder,
    const Image& image,
    std::vector<std::shared_ptr<const TrackInfo>>& trackInfos)
{
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const TrackInfo>& trackInfo)
        {
            auto sectors = encoder.collectSectors(trackInfo, image);
            return encoder.encode(trackInfo, sectors, image);
        },
        [](const auto&)
        {
            return true;
        },
        trackInfos);
}

void writeTracksAndVerify(FluxSink& fluxSink,
    Encoder& encoder,
    FluxSource& fluxSource,
    Decoder& decoder,
    const Image& image,
    std::vector<std::shared_ptr<const TrackInfo>>& trackInfos)
{
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const TrackInfo>& trackInfo)
        {
            auto sectors = encoder.collectSectors(trackInfo, image);
            return encoder.encode(trackInfo, sectors, image);
        },
        [&](std::shared_ptr<const TrackInfo>& trackInfo)
        {
            auto trackFlux = std::make_shared<TrackFlux>();
            trackFlux->trackInfo = trackInfo;
            FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
            auto result = readGroup(
                fluxSourceIteratorHolder, trackInfo, *trackFlux, decoder);
            log(TrackReadLogMessage{trackFlux});

            if (result != GOOD_READ)
            {
                adjustTrackOnError(fluxSource, trackInfo->physicalTrack);
                log("bad read");
                return false;
            }

            Image wanted;
            for (const auto& sector : encoder.collectSectors(trackInfo, image))
                wanted
                    .put(sector->logicalTrack,
                        sector->logicalSide,
                        sector->logicalSector)
                    ->data = sector->data;

            for (const auto& sector : trackFlux->sectors)
            {
                const auto s = wanted.get(sector->logicalTrack,
                    sector->logicalSide,
                    sector->logicalSector);
                if (!s)
                {
                    log("spurious sector on verify");
                    return false;
                }
                if (s->data != sector->data.slice(0, s->data.size()))
                {
                    log("data mismatch on verify");
                    return false;
                }
                wanted.erase(sector->logicalTrack,
                    sector->logicalSide,
                    sector->logicalSector);
            }
            if (!wanted.empty())
            {
                log("missing sector on verify");
                return false;
            }
            return true;
        },
        trackInfos);
}

void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource,
    std::vector<std::shared_ptr<const TrackInfo>>& locations)
{
    if (fluxSource && decoder)
        writeTracksAndVerify(
            fluxSink, encoder, *fluxSource, *decoder, image, locations);
    else
        writeTracks(fluxSink, encoder, image, locations);
}

void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource)
{
    auto locations = Layout::computeLocations();
    writeDiskCommand(image, encoder, fluxSink, decoder, fluxSource, locations);
}

void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink)
{
    auto locations = Layout::computeLocations();
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const TrackInfo>& trackInfo)
        {
            return fluxSource
                .readFlux(trackInfo->physicalTrack, trackInfo->physicalSide)
                ->next();
        },
        [](const auto&)
        {
            return true;
        },
        locations);
}

std::shared_ptr<TrackFlux> readAndDecodeTrack(FluxSource& fluxSource,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& trackInfo)
{
    auto trackFlux = std::make_shared<TrackFlux>();
    trackFlux->trackInfo = trackInfo;

    if (fluxSource.isHardware())
        measureDiskRotation();

    FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
    int retriesRemaining = globalConfig()->decoder().retries();
    for (;;)
    {
        auto result =
            readGroup(fluxSourceIteratorHolder, trackInfo, *trackFlux, decoder);
        if (result == GOOD_READ)
            break;
        if (result == BAD_AND_CAN_NOT_RETRY)
        {
            log("no more data; giving up");
            break;
        }

        if (retriesRemaining == 0)
        {
            log("giving up");
            break;
        }

        if (fluxSource.isHardware())
        {
            adjustTrackOnError(fluxSource, trackInfo->physicalTrack);
            log("retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }
    }

    return trackFlux;
}

std::shared_ptr<const DiskFlux> readDiskCommand(
    FluxSource& fluxSource, Decoder& decoder)
{
    std::unique_ptr<FluxSink> outputFluxSink;
    if (globalConfig()->decoder().has_copy_flux_to())
        outputFluxSink =
            FluxSink::create(globalConfig()->decoder().copy_flux_to());

    auto diskflux = std::make_shared<DiskFlux>();

    log(BeginOperationLogMessage{"Reading and decoding disk"});
    auto locations = Layout::computeLocations();
    unsigned index = 0;
    for (auto& trackInfo : locations)
    {
        log(OperationProgressLogMessage{
            index * 100 / (unsigned)locations.size()});
        index++;

        testForEmergencyStop();

        auto trackFlux = readAndDecodeTrack(fluxSource, decoder, trackInfo);
        diskflux->tracks.push_back(trackFlux);

        if (outputFluxSink)
        {
            for (const auto& data : trackFlux->trackDatas)
                outputFluxSink->writeFlux(trackInfo->physicalTrack,
                    trackInfo->physicalSide,
                    *data->fluxmap);
        }

        if (globalConfig()->decoder().dump_records())
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

        if (globalConfig()->decoder().dump_sectors())
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
        log(TrackReadLogMessage{trackFlux});
    }

    std::set<std::shared_ptr<const Sector>> all_sectors;
    for (auto& track : diskflux->tracks)
        for (auto& sector : track->sectors)
            all_sectors.insert(sector);
    all_sectors = collectSectors(all_sectors);
    diskflux->image = std::make_shared<Image>(all_sectors);

    /* diskflux can't be modified below this point. */
    log(DiskReadLogMessage{diskflux});
    log(EndOperationLogMessage{"Read complete"});
    return diskflux;
}

void readDiskCommand(
    FluxSource& fluxsource, Decoder& decoder, ImageWriter& writer)
{
    auto diskflux = readDiskCommand(fluxsource, decoder);

    writer.printMap(*diskflux->image);
    if (globalConfig()->decoder().has_write_csv_to())
        writer.writeCsv(
            *diskflux->image, globalConfig()->decoder().write_csv_to());
    writer.writeMappedImage(*diskflux->image);
}

void rawReadDiskCommand(FluxSource& fluxsource, FluxSink& fluxsink)
{
    log(BeginOperationLogMessage{"Performing raw read of disk"});

    if (fluxsource.isHardware() || fluxsink.isHardware())
        measureDiskRotation();
    auto locations = Layout::computeLocations();
    unsigned index = 0;
    for (auto& trackInfo : locations)
    {
        log(OperationProgressLogMessage{index * 100 / (int)locations.size()});
        index++;

        testForEmergencyStop();
        auto fluxSourceIterator = fluxsource.readFlux(
            trackInfo->physicalTrack, trackInfo->physicalSide);

        log(BeginReadOperationLogMessage{
            trackInfo->physicalTrack, trackInfo->physicalSide});
        auto fluxmap = fluxSourceIterator->next();
        log(EndReadOperationLogMessage());
        log("{0} ms in {1} bytes",
            (int)(fluxmap->duration() / 1e6),
            fluxmap->bytes());

        fluxsink.writeFlux(
            trackInfo->physicalTrack, trackInfo->physicalSide, *fluxmap);
    }

    log(EndOperationLogMessage{"Raw read complete"});
}
