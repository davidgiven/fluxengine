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
    std::set<std::shared_ptr<const Sector>> rawSectors;
    std::set<std::shared_ptr<const Record>> rawRecords;
    for (const auto& trackDataFlux : m->fluxes)
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
        m->sectors.begin(), m->sectors.end());
    std::sort(sectors.begin(), sectors.end(), sectorPointerSortPredicate);

    for (const auto& sector : sectors)
        r.add(fmt::format("{}.{}.{}{}",
            sector->logicalCylinder,
            sector->logicalHead,
            sector->logicalSector,
            Sector::statusToChar(sector->status)));

    int size = 0;
    std::set<std::pair<int, int>> track_ids;
    for (const auto& sector : m->sectors)
    {
        track_ids.insert(
            std::make_pair(sector->logicalCylinder, sector->logicalHead));
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

static std::vector<std::shared_ptr<const Sector>> collectSectors(
    std::vector<std::shared_ptr<const Sector>>& trackSectors,
    bool collapse_conflicts = true)
{
    typedef std::tuple<unsigned, unsigned, unsigned> key_t;
    std::multimap<key_t, std::shared_ptr<const Sector>> sectors;

    for (const auto& sector : trackSectors)
    {
        key_t sectorid = {sector->logicalCylinder,
            sector->logicalHead,
            sector->logicalSector};
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
    return sector_set | std::ranges::to<std::vector>();
}

struct CombinationResult
{
    BadSectorsState result;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

static CombinationResult combineRecordAndSectors(
    std::vector<std::shared_ptr<const TrackDataFlux>>& fluxes,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& trackLayout)
{
    CombinationResult cr = {HAS_NO_BAD_SECTORS};
    std::vector<std::shared_ptr<const Sector>> track_sectors;

    /* Add the sectors which were there. */

    for (auto& trackdataflux : fluxes)
        for (auto& sector : trackdataflux->sectors)
            track_sectors.push_back(sector);

    /* Add the sectors which should be there. */

    for (unsigned sectorId : trackLayout->naturalSectorOrder)
    {
        auto sector = std::make_shared<Sector>(trackLayout,
            LogicalLocation{trackLayout->logicalCylinder,
                trackLayout->logicalHead,
                sectorId});

        sector->status = Sector::MISSING;
        track_sectors.push_back(sector);
    }

    /* Deduplicate. */

    cr.sectors = collectSectors(track_sectors);
    if (cr.sectors.empty())
        cr.result = HAS_BAD_SECTORS;
    for (const auto& sector : cr.sectors)
        if (sector->status != Sector::OK)
            cr.result = HAS_BAD_SECTORS;

    return cr;
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

struct ReadGroupResult
{
    ReadResult result;
    std::vector<std::shared_ptr<const Sector>> sectors;
};

static ReadGroupResult readGroup(
    FluxSourceIteratorHolder& fluxSourceIteratorHolder,
    std::shared_ptr<const TrackInfo>& trackInfo,
    std::vector<std::shared_ptr<const TrackDataFlux>>& fluxes,
    Decoder& decoder)
{
    ReadGroupResult rgr = {BAD_AND_CAN_NOT_RETRY};

    for (unsigned offset = 0; offset < trackInfo->groupSize;
        offset += Layout::getHeadWidth())
    {
        /* Do the physical read. */

        log(BeginReadOperationLogMessage{
            trackInfo->physicalCylinder + offset, trackInfo->physicalHead});

        auto& fluxSourceIterator = fluxSourceIteratorHolder.getIterator(
            trackInfo->physicalCylinder + offset, trackInfo->physicalHead);
        if (!fluxSourceIterator.hasNext())
            continue;

        auto fluxmap = fluxSourceIterator.next();
        log(EndReadOperationLogMessage());
        log("{0} ms in {1} bytes",
            (int)(fluxmap->duration() / 1e6),
            fluxmap->bytes());

        auto flux = decoder.decodeToSectors(std::move(fluxmap), trackInfo);
        fluxes.push_back(flux);

        /* Decode what we've got so far. */

        auto [result, sectors] =
            combineRecordAndSectors(fluxes, decoder, trackInfo);
        rgr.sectors = sectors;
        if (result == HAS_NO_BAD_SECTORS)
        {
            /* We have all necessary sectors, so can stop here. */
            rgr.result = GOOD_READ;
            if (globalConfig()->decoder().skip_unnecessary_tracks())
                break;
        }
        else if (fluxSourceIterator.hasNext())
        {
            /* The flux source claims it can do more reads, so mark this group
             * as being retryable. */
            rgr.result = BAD_AND_CAN_RETRY;
        }
    }

    return rgr;
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
                unsigned physicalCylinder =
                    trackInfo->physicalCylinder + offset;

                log(BeginWriteOperationLogMessage{
                    physicalCylinder, trackInfo->physicalHead});

                if (offset == globalConfig()->drive().group_offset())
                {
                    auto fluxmap = producer(trackInfo);
                    if (!fluxmap)
                        goto erase;

                    fluxSink.writeFlux(
                        physicalCylinder, trackInfo->physicalHead, *fluxmap);
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
                        physicalCylinder, trackInfo->physicalHead, blank);
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
            FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
            std::vector<std::shared_ptr<const TrackDataFlux>> fluxes;
            auto [result, sectors] =
                readGroup(fluxSourceIteratorHolder, trackInfo, fluxes, decoder);
            log(TrackReadLogMessage{fluxes, sectors});

            if (result != GOOD_READ)
            {
                adjustTrackOnError(fluxSource, trackInfo->physicalCylinder);
                log("bad read");
                return false;
            }

            Image wanted;
            for (const auto& sector : encoder.collectSectors(trackInfo, image))
                wanted
                    .put(sector->logicalCylinder,
                        sector->logicalHead,
                        sector->logicalSector)
                    ->data = sector->data;

            for (const auto& sector : sectors)
            {
                const auto s = wanted.get(sector->logicalCylinder,
                    sector->logicalHead,
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
                wanted.erase(sector->logicalCylinder,
                    sector->logicalHead,
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
    const std::vector<CylinderHead>& physicalLocations)
{
    auto trackinfos = Layout::getLayoutOfTracksPhysical(physicalLocations);
    if (fluxSource && decoder)
        writeTracksAndVerify(
            fluxSink, encoder, *fluxSource, *decoder, image, trackinfos);
    else
        writeTracks(fluxSink, encoder, image, trackinfos);
}

void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource)
{
    auto locations = Layout::computePhysicalLocations();
    writeDiskCommand(image, encoder, fluxSink, decoder, fluxSource, locations);
}

void writeRawDiskCommand(FluxSource& fluxSource, FluxSink& fluxSink)
{
    auto physicalLocations = Layout::computePhysicalLocations();
    auto trackinfos = Layout::getLayoutOfTracksPhysical(physicalLocations);
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const TrackInfo>& trackInfo)
        {
            return fluxSource
                .readFlux(trackInfo->physicalCylinder, trackInfo->physicalHead)
                ->next();
        },
        [](const auto&)
        {
            return true;
        },
        trackinfos);
}

FluxAndSectors readAndDecodeTrack(FluxSource& fluxSource,
    Decoder& decoder,
    std::shared_ptr<const TrackInfo>& trackInfo)
{
    FluxAndSectors fas;

    if (fluxSource.isHardware())
        measureDiskRotation();

    FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
    int retriesRemaining = globalConfig()->decoder().retries();
    for (;;)
    {
        auto [result, sectors] =
            readGroup(fluxSourceIteratorHolder, trackInfo, fas.fluxes, decoder);
        std::copy(
            sectors.begin(), sectors.end(), std::back_inserter(fas.sectors));
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
            adjustTrackOnError(fluxSource, trackInfo->physicalCylinder);
            log("retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }
    }

    return fas;
}

void readDiskCommand(
    FluxSource& fluxSource, Decoder& decoder, DiskFlux& diskflux)
{
    std::unique_ptr<FluxSink> outputFluxSink;
    if (globalConfig()->decoder().has_copy_flux_to())
        outputFluxSink =
            FluxSink::create(globalConfig()->decoder().copy_flux_to());

    if (!diskflux.layout)
        diskflux.layout = createDiskLayout();

    log(BeginOperationLogMessage{"Reading and decoding disk"});
    auto physicalLocations = Layout::computePhysicalLocations();
    unsigned index = 0;
    for (auto physicalLocation : physicalLocations)
    {
        auto trackInfo = Layout::getLayoutOfTrackPhysical(
            physicalLocation.cylinder, physicalLocation.head);

        log(OperationProgressLogMessage{
            index * 100 / (unsigned)physicalLocations.size()});
        index++;

        testForEmergencyStop();

        auto [trackFluxes, trackSectors] =
            readAndDecodeTrack(fluxSource, decoder, trackInfo);
        for (const auto& flux : trackFluxes)
            diskflux.fluxesByTrack.insert(std::pair{physicalLocation, flux});
        for (const auto& sector : trackSectors)
            diskflux.sectorsByTrack.insert(std::pair{physicalLocation, sector});

        if (outputFluxSink)
        {
            for (const auto& data : trackFluxes)
                outputFluxSink->writeFlux(trackInfo->physicalCylinder,
                    trackInfo->physicalHead,
                    *data->fluxmap);
        }

        if (globalConfig()->decoder().dump_records())
        {
            std::vector<std::shared_ptr<const Record>> sorted_records;

            for (const auto& data : trackFluxes)
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
            auto sectors = collectSectors(trackSectors, false);
            std::ranges::sort(sectors,
                [](const auto& o1, const auto& o2)
                {
                    return *o1 < *o2;
                });

            std::cout << "\nDecoded sectors follow:\n\n";
            for (const auto& sector : sectors)
            {
                std::cout << fmt::format(
                    "{}.{:02}.{:02}: I+{:.2f}us with {:.2f}us clock: "
                    "status {}\n",
                    sector->logicalCylinder,
                    sector->logicalHead,
                    sector->logicalSector,
                    sector->headerStartTime / 1000.0,
                    sector->clock / 1000.0,
                    Sector::statusToString(sector->status));
                hexdump(std::cout, sector->data);
                std::cout << std::endl;
            }
        }

        /* track can't be modified below this point. */
        log(TrackReadLogMessage{trackFluxes, trackSectors});

        std::vector<std::shared_ptr<const Sector>> all_sectors;
        for (auto& [ch, sector] : diskflux.sectorsByTrack)
            all_sectors.push_back(sector);
        all_sectors = collectSectors(all_sectors);
        diskflux.image = std::make_shared<Image>(all_sectors);

        /* Log a _copy_ of the diskflux structure so that the logger doesn't see
         * the diskflux get mutated in subsequent reads. */
        log(DiskReadLogMessage{std::make_shared<DiskFlux>(diskflux)});
    }

    if (!diskflux.image)
        diskflux.image = std::make_shared<Image>();

    log(EndOperationLogMessage{"Read complete"});
}

void readDiskCommand(
    FluxSource& fluxsource, Decoder& decoder, ImageWriter& writer)
{
    DiskFlux diskflux;
    readDiskCommand(fluxsource, decoder, diskflux);

    writer.printMap(*diskflux.image);
    if (globalConfig()->decoder().has_write_csv_to())
        writer.writeCsv(
            *diskflux.image, globalConfig()->decoder().write_csv_to());
    writer.writeImage(*diskflux.image);
}
