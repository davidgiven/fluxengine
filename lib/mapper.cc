#include "globals.h"
#include "sector.h"
#include "image.h"
#include "fmt/format.h"
#include "logger.h"
#include "proto.h"
#include "mapper.h"
#include "flux.h"
#include "lib/mapper.pb.h"

typedef std::function<void(
    std::map<int, int>&, const SectorMappingProto::MappingProto&)>
    insertercb_t;

static void getTrackFormat(const SectorMappingProto& proto,
    SectorMappingProto::TrackdataProto& trackdata,
    unsigned track,
    unsigned side)
{
    trackdata.Clear();
    for (const SectorMappingProto::TrackdataProto& f : proto.trackdata())
    {
        if (f.has_track() && f.has_up_to_track() &&
            ((track < f.track()) || (track > f.up_to_track())))
            continue;
        if (f.has_track() && !f.has_up_to_track() && (track != f.track()))
            continue;
        if (f.has_side() && (f.side() != side))
            continue;

        trackdata.MergeFrom(f);
    }
}

static std::unique_ptr<Image> remapImpl(const Image& source,
    const SectorMappingProto& mapping,
    insertercb_t inserter_cb)
{
    typedef std::pair<int, int> tracksidekey_t;
    std::map<tracksidekey_t, std::map<int, int>> cache;

    auto getTrackdata =
        [&](const tracksidekey_t& key) -> const std::map<int, int>&
    {
        auto it = cache.find(key);
        if (it != cache.end())
            return it->second;

        SectorMappingProto::TrackdataProto trackdata;
        getTrackFormat(mapping, trackdata, key.first, key.second);

        auto& map = cache[key];
        for (const auto mappingsit : trackdata.mapping())
            inserter_cb(map, mappingsit);

        return map;
    };

    std::set<std::shared_ptr<const Sector>> destSectors;
    for (const auto& sector : source)
    {
        tracksidekey_t key = {sector->logicalTrack, sector->logicalSide};
        const auto& trackdata = getTrackdata(key);
        if (trackdata.empty())
            destSectors.insert(sector);
        else
        {
            auto it = trackdata.find(sector->physicalSector);
            if (it == trackdata.end())
                Error() << fmt::format(
                    "mapping requested but mapping table has no entry for "
                    "sector {}",
                    sector->physicalSector);

            auto newSector = std::make_shared<Sector>(*sector);
            newSector->physicalSector = it->second;
            destSectors.insert(newSector);
        }
    }

    return std::make_unique<Image>(destSectors);
}

std::unique_ptr<const Image> Mapper::remapSectorsPhysicalToLogical(
    const Image& source, const SectorMappingProto& mapping)
{
    Logger() << "remapping sectors from physical IDs to logical IDs";
    return remapImpl(source,
        mapping,
        [](auto& map, const auto& pair)
        {
            map.insert({pair.physical(), pair.logical()});
        });
}

std::unique_ptr<const Image> Mapper::remapSectorsLogicalToPhysical(
    const Image& source, const SectorMappingProto& mapping)
{
    Logger() << "remapping sectors from logical IDs to physical IDs";
    return remapImpl(source,
        mapping,
        [](auto& map, const auto& pair)
        {
            map.insert({pair.logical(), pair.physical()});
        });
}

unsigned Mapper::remapTrackPhysicalToLogical(unsigned ptrack)
{
    return (ptrack - config.drive().head_bias()) / config.drive().head_width();
}

static unsigned getTrackStep()
{
    unsigned track_step =
        (config.tpi() == 0) ? 1 : (config.drive().tpi() / config.tpi());

    if (track_step == 0)
        Error()
            << "this drive can't write this image, because the head is too big";
    return track_step;
}

unsigned Mapper::remapTrackLogicalToPhysical(unsigned ltrack)
{
    return config.drive().head_bias() + ltrack * getTrackStep();
}

std::set<Location> Mapper::computeLocations()
{
    std::set<Location> locations;

    for (unsigned logicalTrack : iterate(config.tracks()))
    {
        for (unsigned head : iterate(config.heads()))
            locations.insert(computeLocationFor(logicalTrack, head));
    }

    return locations;
}

Location Mapper::computeLocationFor(unsigned logicalTrack, unsigned logicalHead)
{
    if ((logicalTrack < config.layout().tracks()) &&
        (logicalHead < config.layout().sides()))
    {
        unsigned track_step = getTrackStep();
        unsigned physicalTrack =
            config.drive().head_bias() + logicalTrack * track_step;

        return {.physicalTrack = physicalTrack,
            .logicalTrack = logicalTrack,
            .head = logicalHead,
            .groupSize = track_step};
    }

    Error() << fmt::format(
        "track {}.{} is not part of the image", logicalTrack, logicalHead);
}

nanoseconds_t Mapper::calculatePhysicalClockPeriod(
    nanoseconds_t targetClockPeriod, nanoseconds_t targetRotationalPeriod)
{
    nanoseconds_t currentRotationalPeriod =
        config.drive().rotational_period_ms() * 1e6;
    if (currentRotationalPeriod == 0)
        Error() << "you must set --drive.rotational_period_ms as it can't be "
                   "autodetected";

    return targetClockPeriod *
           (currentRotationalPeriod / targetRotationalPeriod);
}
