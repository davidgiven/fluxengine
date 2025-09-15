#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/layout.h"
#include "lib/config/proto.h"
#include "lib/core/logger.h"

static unsigned getTrackStep()
{
    auto format_type = globalConfig()->layout().format_type();
    auto drive_type = globalConfig()->drive().drive_type();

    switch (format_type)
    {
        case FORMATTYPE_40TRACK:
            switch (drive_type)
            {
                case DRIVETYPE_40TRACK:
                    return 1;

                case DRIVETYPE_80TRACK:
                    return 2;

                case DRIVETYPE_APPLE2:
                    return 4;
            }

        case FORMATTYPE_80TRACK:
            switch (drive_type)
            {
                case DRIVETYPE_40TRACK:
                    error(
                        "you can't read/write an 80 track image from/to a 40 "
                        "track "
                        "drive");

                case DRIVETYPE_80TRACK:
                    return 1;

                case DRIVETYPE_APPLE2:
                    error(
                        "you can't read/write an 80 track image from/to an "
                        "Apple II "
                        "drive");
            }
    }

    return 1;
}

unsigned Layout::remapCylinderPhysicalToLogical(unsigned ptrack)
{
    return (ptrack - globalConfig()->drive().head_bias()) / getTrackStep();
}

unsigned Layout::remapCylinderLogicalToPhysical(unsigned ltrack)
{
    return globalConfig()->drive().head_bias() + ltrack * getTrackStep();
}

unsigned Layout::remapHeadPhysicalToLogical(unsigned pside)
{
    return pside ^ globalConfig()->layout().swap_sides();
}

unsigned Layout::remapHeadLogicalToPhysical(unsigned lside)
{
    return lside ^ globalConfig()->layout().swap_sides();
}

std::vector<CylinderHead> Layout::computePhysicalLocations()
{
    if (!globalConfig()->tracks().empty())
        return parseCylinderHeadsString(globalConfig()->tracks());

    std::set<unsigned> tracks = iterate(0, globalConfig()->layout().tracks());
    std::set<unsigned> heads = iterate(0, globalConfig()->layout().sides());

    std::vector<CylinderHead> locations;
    for (unsigned logicalCylinder : tracks)
        for (unsigned logicalHead : heads)
            locations.push_back(
                CylinderHead{remapCylinderLogicalToPhysical(logicalCylinder),
                    remapHeadLogicalToPhysical(logicalHead)});

    return locations;
}

std::vector<CylinderHead> Layout::computeLogicalLocations()
{
    if (!globalConfig()->tracks().empty())
        return parseCylinderHeadsString(globalConfig()->tracks());

    std::set<unsigned> tracks = iterate(0, globalConfig()->layout().tracks());
    std::set<unsigned> heads = iterate(0, globalConfig()->layout().sides());

    std::vector<CylinderHead> locations;
    for (unsigned logicalCylinder : tracks)
        for (unsigned logicalHead : heads)
            locations.push_back(CylinderHead{logicalCylinder, logicalHead});

    return locations;
}

Layout::LayoutBounds Layout::getBounds(
    const std::vector<CylinderHead>& locations)
{
    LayoutBounds r{.minCylinder = INT_MAX,
        .maxCylinder = INT_MIN,
        .minHead = INT_MAX,
        .maxHead = INT_MIN};

    for (const auto& ti : locations)
    {
        r.minCylinder = std::min<int>(r.minCylinder, ti.cylinder);
        r.maxCylinder = std::max<int>(r.maxCylinder, ti.cylinder);
        r.minHead = std::min<int>(r.minHead, ti.head);
        r.maxHead = std::max<int>(r.maxHead, ti.head);
    }

    return r;
}

std::vector<std::pair<int, int>> Layout::getTrackOrdering(
    LayoutProto::Order ordering,
    unsigned guessedCylinders,
    unsigned guessedHeads)
{
    auto layout = globalConfig()->layout();
    int tracks = layout.has_tracks() ? layout.tracks() : guessedCylinders;
    int sides = layout.has_sides() ? layout.sides() : guessedHeads;

    std::vector<std::pair<int, int>> trackList;
    switch (ordering)
    {
        case LayoutProto::CHS:
        {
            for (int track = 0; track < tracks; track++)
            {
                for (int side = 0; side < sides; side++)
                    trackList.push_back(std::make_pair(track, side));
            }
            break;
        }

        case LayoutProto::HCS:
        {
            for (int side = 0; side < sides; side++)
            {
                for (int track = 0; track < tracks; track++)
                    trackList.push_back(std::make_pair(track, side));
            }
            break;
        }

        case LayoutProto::HCS_RH1:
        {
            for (int side = 0; side < sides; side++)
            {
                if (side == 0)
                    for (int track = 0; track < tracks; track++)
                        trackList.push_back(std::make_pair(track, side));
                if (side == 1)
                    for (int track = tracks; track >= 0; track--)
                        trackList.push_back(std::make_pair(track - 1, side));
            }
            break;
        }

        default:
            error("LAYOUT: invalid track trackList");
    }

    return trackList;
}

std::vector<unsigned> Layout::expandSectorList(
    const SectorListProto& sectorsProto)
{
    std::vector<unsigned> sectors;

    if (sectorsProto.has_count())
    {
        if (sectorsProto.sector_size() != 0)
            error(
                "LAYOUT: if you use a sector count, you can't use an "
                "explicit sector list");

        std::set<unsigned> sectorset;
        int id = sectorsProto.start_sector();
        for (int i = 0; i < sectorsProto.count(); i++)
        {
            while (sectorset.find(id) != sectorset.end())
            {
                id++;
                if (id >= (sectorsProto.start_sector() + sectorsProto.count()))
                    id -= sectorsProto.count();
            }

            sectorset.insert(id);
            sectors.push_back(id);

            id += sectorsProto.skew();
            if (id >= (sectorsProto.start_sector() + sectorsProto.count()))
                id -= sectorsProto.count();
        }
    }
    else if (sectorsProto.sector_size() > 0)
    {
        for (int sectorId : sectorsProto.sector())
            sectors.push_back(sectorId);
    }
    else
        error("LAYOUT: no sectors in sector definition!");

    return sectors;
}

std::shared_ptr<const TrackInfo> Layout::getLayoutOfTrack(
    unsigned logicalCylinder, unsigned logicalHead)
{
    auto trackInfo = std::make_shared<TrackInfo>();

    LayoutProto::LayoutdataProto layoutdata;
    for (const auto& f : globalConfig()->layout().layoutdata())
    {
        if (f.has_track() && f.has_up_to_track() &&
            ((logicalCylinder < f.track()) ||
                (logicalCylinder > f.up_to_track())))
            continue;
        if (f.has_track() && !f.has_up_to_track() &&
            (logicalCylinder != f.track()))
            continue;
        if (f.has_side() && (f.side() != logicalHead))
            continue;

        layoutdata.MergeFrom(f);
    }

    trackInfo->numCylinders = globalConfig()->layout().tracks();
    trackInfo->numHeads = globalConfig()->layout().sides();
    trackInfo->sectorSize = layoutdata.sector_size();
    trackInfo->logicalCylinder = logicalCylinder;
    trackInfo->logicalHead = logicalHead;
    trackInfo->physicalCylinder =
        remapCylinderLogicalToPhysical(logicalCylinder);
    trackInfo->physicalHead =
        logicalHead ^ globalConfig()->layout().swap_sides();
    trackInfo->groupSize = getTrackStep();
    trackInfo->diskSectorOrder = expandSectorList(layoutdata.physical());
    trackInfo->naturalSectorOrder = trackInfo->diskSectorOrder;
    std::sort(trackInfo->naturalSectorOrder.begin(),
        trackInfo->naturalSectorOrder.end());
    trackInfo->numSectors = trackInfo->naturalSectorOrder.size();

    if (layoutdata.has_filesystem())
    {
        trackInfo->filesystemSectorOrder =
            expandSectorList(layoutdata.filesystem());
        if (trackInfo->filesystemSectorOrder.size() != trackInfo->numSectors)
            error(
                "filesystem sector order list doesn't contain the right "
                "number of sectors");
    }
    else
        trackInfo->filesystemSectorOrder = trackInfo->naturalSectorOrder;

    for (int i = 0; i < trackInfo->numSectors; i++)
    {
        unsigned fid = trackInfo->naturalSectorOrder[i];
        unsigned lid = trackInfo->filesystemSectorOrder[i];
        trackInfo->filesystemToNaturalSectorMap[fid] = lid;
        trackInfo->naturalToFilesystemSectorMap[lid] = fid;
    }

    return trackInfo;
}

std::shared_ptr<const TrackInfo> Layout::getLayoutOfTrackPhysical(
    unsigned physicalCylinder, unsigned physicalHead)
{
    return getLayoutOfTrack(remapCylinderPhysicalToLogical(physicalCylinder),
        remapHeadPhysicalToLogical(physicalHead));
}

std::shared_ptr<const TrackInfo> Layout::getLayoutOfTrackPhysical(
    const CylinderHead& physicalLocation)
{
    return getLayoutOfTrackPhysical(
        physicalLocation.cylinder, physicalLocation.head);
}

std::vector<std::shared_ptr<const TrackInfo>> Layout::getLayoutOfTracksPhysical(
    const std::vector<CylinderHead>& physicalLocations)
{
    std::vector<std::shared_ptr<const TrackInfo>> results;
    for (const auto& physicalLocation : physicalLocations)
        results.push_back(getLayoutOfTrackPhysical(physicalLocation));
    return results;
}

int Layout::getHeadWidth()
{
    switch (globalConfig()->drive().drive_type())
    {
        case DRIVETYPE_APPLE2:
            return 4;

        default:
            return 1;
    }
}

std::vector<LogicalLocation> Layout::computeFilesystemLogicalOrdering()
{
    std::vector<LogicalLocation> result;
    auto& layout = globalConfig()->layout();
    if (layout.has_tracks() && layout.has_sides())
    {
        unsigned block = 0;
        for (const auto& p :
            Layout::getTrackOrdering(layout.filesystem_track_order(),
                layout.tracks(),
                layout.sides()))
        {
            unsigned track = p.first;
            unsigned side = p.second;

            auto trackLayout = Layout::getLayoutOfTrack(track, side);
            if (trackLayout->numSectors == 0)
                continue;

            for (unsigned sectorId : trackLayout->filesystemSectorOrder)
                result.push_back({track, side, sectorId});
        }
    }
    return result;
}