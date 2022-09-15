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
#include "layout.h"
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

void measureDiskRotation(
    nanoseconds_t& oneRevolution, nanoseconds_t& hardSectorThreshold)
{
    Logger() << BeginSpeedOperationLogMessage();

    int retries = 5;
    usbSetDrive(config.drive().drive(),
        config.drive().high_density(),
        config.drive().index_mode());
    oneRevolution = config.drive().rotational_period_ms() * 1e6;
    if (config.drive().hard_sector_count() != 0)
        hardSectorThreshold =
            oneRevolution * 3 / (4 * config.drive().hard_sector_count());
    else
        hardSectorThreshold = 0;

    if (oneRevolution == 0)
    {
        Logger() << BeginOperationLogMessage{
            "Measuring drive rotational speed"};
        do
        {
            oneRevolution =
                usbGetRotationalPeriod(config.drive().hard_sector_count());
            if (config.drive().hard_sector_count() != 0)
                hardSectorThreshold = oneRevolution * 3 /
                                      (4 * config.drive().hard_sector_count());

            retries--;
        } while ((oneRevolution == 0) && (retries > 0));
        config.mutable_drive()->set_rotational_period_ms(oneRevolution / 1e6);
        Logger() << EndOperationLogMessage{};
    }

    if (oneRevolution == 0)
        Error() << "Failed\nIs a disk in the drive?";

    Logger() << EndSpeedOperationLogMessage{oneRevolution};
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
    std::shared_ptr<const Track>& layout)
{
    std::set<std::shared_ptr<const Sector>> track_sectors;

    for (auto& trackdataflux : trackFlux.trackDatas)
        track_sectors.insert(
            trackdataflux->sectors.begin(), trackdataflux->sectors.end());

    for (auto& logicalLocation : decoder.requiredSectors(trackFlux.layout))
    {
        auto sector = std::make_shared<Sector>(logicalLocation);
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
    std::shared_ptr<const Track>& layout,
    TrackFlux& trackFlux,
    Decoder& decoder)
{
    ReadResult result = BAD_AND_CAN_NOT_RETRY;

    for (unsigned offset = 0; offset < layout->groupSize;
         offset += config.drive().head_width())
    {
        auto& fluxSourceIterator = fluxSourceIteratorHolder.getIterator(
            layout->physicalTrack + offset, layout->physicalSide);
        if (!fluxSourceIterator.hasNext())
            continue;

        Logger() << BeginReadOperationLogMessage{
            layout->physicalTrack + offset, layout->physicalSide};
        std::shared_ptr<const Fluxmap> fluxmap = fluxSourceIterator.next();
        // ->rescale(
        //     1.0 / config.flux_source().rescale());
        Logger() << EndReadOperationLogMessage()
                 << fmt::format("{0} ms in {1} bytes",
                        (int)(fluxmap->duration() / 1e6),
                        fluxmap->bytes());

        auto trackdataflux = decoder.decodeToSectors(fluxmap, layout);
        trackFlux.trackDatas.push_back(trackdataflux);
        if (combineRecordAndSectors(trackFlux, decoder, layout) ==
            HAS_NO_BAD_SECTORS)
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
    std::function<std::unique_ptr<const Fluxmap>(
        std::shared_ptr<const Track>& layout)> producer,
    std::function<bool(std::shared_ptr<const Track>& layout)> verifier,
    std::vector<std::shared_ptr<const Track>>& layouts)
{
    Logger() << BeginOperationLogMessage{"Encoding and writing to disk"};

    int index = 0;
    for (auto& layout : layouts)
    {
        Logger() << OperationProgressLogMessage{
            index * 100 / (unsigned)layouts.size()};
        index++;

        testForEmergencyStop();

        int retriesRemaining = config.decoder().retries();
        for (;;)
        {
            for (int offset = 0; offset < layout->groupSize;
                 offset += config.drive().head_width())
            {
                unsigned physicalTrack = layout->physicalTrack + offset;

                Logger() << BeginWriteOperationLogMessage{
                    physicalTrack, layout->physicalSide};

                if (offset == config.drive().group_offset())
                {
                    auto fluxmap = producer(layout);
                    if (!fluxmap)
                        goto erase;

                    fluxSink.writeFlux(
                        physicalTrack, layout->physicalSide, *fluxmap);
                    Logger() << fmt::format("writing {0} ms in {1} bytes",
                        int(fluxmap->duration() / 1e6),
                        fluxmap->bytes());
                }
                else
                {
                erase:
                    /* Erase this track rather than writing. */

                    Fluxmap blank;
                    fluxSink.writeFlux(
                        physicalTrack, layout->physicalSide, blank);
                    Logger() << "erased";
                }

                Logger() << EndWriteOperationLogMessage();
            }

            if (verifier(layout))
                break;

            if (retriesRemaining == 0)
                Error() << "fatal error on write";

            Logger() << fmt::format(
                "retrying; {} retries remaining", retriesRemaining);
            retriesRemaining--;
        }
    }

    Logger() << EndOperationLogMessage{"Write complete"};
}

void writeTracks(FluxSink& fluxSink,
    Encoder& encoder,
    const Image& image,
    std::vector<std::shared_ptr<const Track>>& layouts)
{
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const Track>& layout)
        {
            auto sectors = encoder.collectSectors(layout, image);
            return encoder.encode(layout, sectors, image);
        },
        [](const auto&)
        {
            return true;
        },
        layouts);
}

void writeTracksAndVerify(FluxSink& fluxSink,
    Encoder& encoder,
    FluxSource& fluxSource,
    Decoder& decoder,
    const Image& image,
    std::vector<std::shared_ptr<const Track>>& locations)
{
    writeTracks(
        fluxSink,
        [&](std::shared_ptr<const Track>& layout)
        {
            auto sectors = encoder.collectSectors(layout, image);
            return encoder.encode(layout, sectors, image);
        },
        [&](std::shared_ptr<const Track>& layout)
        {
            auto trackFlux = std::make_shared<TrackFlux>();
            trackFlux->layout = layout;
            FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
            auto result = readGroup(
                fluxSourceIteratorHolder, layout, *trackFlux, decoder);
            Logger() << TrackReadLogMessage{trackFlux};

            if (result != GOOD_READ)
            {
                Logger() << "bad read";
                return false;
            }

            Image wanted;
            for (const auto& sector : encoder.collectSectors(layout, image))
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
                    Logger() << "spurious sector on verify";
                    return false;
                }
                if (s->data != sector->data.slice(0, s->data.size()))
                {
                    Logger() << "data mismatch on verify";
                    return false;
                }
                wanted.erase(sector->logicalTrack,
                    sector->logicalSide,
                    sector->logicalSector);
            }
            if (!wanted.empty())
            {
                Logger() << "missing sector on verify";
                return false;
            }
            return true;
        },
        locations);
}

void writeDiskCommand(const Image& image,
    Encoder& encoder,
    FluxSink& fluxSink,
    Decoder* decoder,
    FluxSource* fluxSource,
    std::vector<std::shared_ptr<const Track>>& locations)
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
        [&](std::shared_ptr<const Track>& layout)
        {
            return fluxSource
                .readFlux(layout->physicalTrack, layout->physicalSide)
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
    std::shared_ptr<const Track>& layout)
{
    auto trackFlux = std::make_shared<TrackFlux>();
    trackFlux->layout = layout;

    FluxSourceIteratorHolder fluxSourceIteratorHolder(fluxSource);
    int retriesRemaining = config.decoder().retries();
    for (;;)
    {
        auto result =
            readGroup(fluxSourceIteratorHolder, layout, *trackFlux, decoder);
        if (result == GOOD_READ)
            break;
        if (result == BAD_AND_CAN_NOT_RETRY)
        {
            Logger() << fmt::format("no more data; giving up");
            break;
        }

        if (retriesRemaining == 0)
        {
            Logger() << fmt::format("giving up");
            break;
        }

        Logger() << fmt::format(
            "retrying; {} retries remaining", retriesRemaining);
        retriesRemaining--;
    }

    return trackFlux;
}

std::shared_ptr<const DiskFlux> readDiskCommand(
    FluxSource& fluxSource, Decoder& decoder)
{
    std::unique_ptr<FluxSink> outputFluxSink;
    if (config.decoder().has_copy_flux_to())
        outputFluxSink = FluxSink::create(config.decoder().copy_flux_to());

    auto diskflux = std::make_shared<DiskFlux>();

    Logger() << BeginOperationLogMessage{"Reading and decoding disk"};
    auto locations = Layout::computeLocations();
    unsigned index = 0;
    for (auto& layout : locations)
    {
        Logger() << OperationProgressLogMessage{
            index * 100 / (unsigned)locations.size()};
        index++;

        testForEmergencyStop();

        auto trackFlux = readAndDecodeTrack(fluxSource, decoder, layout);
        diskflux->tracks.push_back(trackFlux);

        if (outputFluxSink)
        {
            for (const auto& data : trackFlux->trackDatas)
                outputFluxSink->writeFlux(layout->physicalTrack,
                    layout->physicalSide,
                    *data->fluxmap);
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

    std::set<std::shared_ptr<const Sector>> all_sectors;
    for (auto& track : diskflux->tracks)
        for (auto& sector : track->sectors)
            all_sectors.insert(sector);
    all_sectors = collectSectors(all_sectors);
    diskflux->image = std::make_shared<Image>(all_sectors);

    /* diskflux can't be modified below this point. */
    Logger() << DiskReadLogMessage{diskflux};
    Logger() << EndOperationLogMessage{"Read complete"};
    return diskflux;
}

void readDiskCommand(
    FluxSource& fluxsource, Decoder& decoder, ImageWriter& writer)
{
    auto diskflux = readDiskCommand(fluxsource, decoder);

    writer.printMap(*diskflux->image);
    if (config.decoder().has_write_csv_to())
        writer.writeCsv(*diskflux->image, config.decoder().write_csv_to());
    writer.writeMappedImage(*diskflux->image);
}

void rawReadDiskCommand(FluxSource& fluxsource, FluxSink& fluxsink)
{
    Logger() << BeginOperationLogMessage{"Performing raw read of disk"};

    auto tracks = iterate(config.tracks());
    auto heads = iterate(config.heads());
    unsigned locations = tracks.size() * heads.size();

    unsigned index = 0;
    for (unsigned track : tracks)
    {
        for (unsigned head : heads)
        {
            Logger() << OperationProgressLogMessage{index * 100 / locations};
            index++;

            testForEmergencyStop();
            auto fluxSourceIterator = fluxsource.readFlux(track, head);

            Logger() << BeginReadOperationLogMessage{track, head};
            auto fluxmap = fluxSourceIterator->next();
            Logger() << EndReadOperationLogMessage()
                     << fmt::format("{0} ms in {1} bytes",
                            (int)(fluxmap->duration() / 1e6),
                            fluxmap->bytes());

            fluxsink.writeFlux(track, head, *fluxmap);
        }
    }

    Logger() << EndOperationLogMessage{"Raw read complete"};
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
