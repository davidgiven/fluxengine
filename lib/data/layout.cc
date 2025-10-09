#include "lib/core/globals.h"
#include "lib/config/config.h"
#include "lib/data/layout.h"
#include "lib/config/proto.h"
#include "lib/core/logger.h"

static unsigned getTrackStep(const ConfigProto& config = globalConfig())
{
    auto format_type = config.layout().format_type();
    auto drive_type = config.drive().drive_type();

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

                default:
                    break;
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

                default:
                    break;
            }

        default:
            break;
    }

    return 1;
}

static std::vector<CylinderHead> getTrackOrdering(
    LayoutProto::Order ordering, unsigned tracks, unsigned sides)
{
    std::vector<CylinderHead> trackList;
    switch (ordering)
    {
        case LayoutProto::CHS:
        {
            for (unsigned track = 0; track < tracks; track++)
            {
                for (unsigned side = 0; side < sides; side++)
                    trackList.push_back({track, side});
            }
            break;
        }

        case LayoutProto::HCS:
        {
            for (unsigned side = 0; side < sides; side++)
            {
                for (unsigned track = 0; track < tracks; track++)
                    trackList.push_back({track, side});
            }
            break;
        }

        case LayoutProto::HCS_RH1:
        {
            for (unsigned side = 0; side < sides; side++)
            {
                if (side == 0)
                    for (unsigned track = 0; track < tracks; track++)
                        trackList.push_back({track, side});
                if (side == 1)
                    for (unsigned track = tracks; track >= 0; track--)
                        trackList.push_back({track - 1, side});
            }
            break;
        }

        default:
            error("LAYOUT: invalid track trackList");
    }

    return trackList;
}

static std::vector<unsigned> expandSectorList(
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

static const LayoutProto::LayoutdataProto getLayoutData(
    unsigned logicalCylinder, unsigned logicalHead, const ConfigProto& config)
{
    LayoutProto::LayoutdataProto layoutData;
    for (const auto& f : config.layout().layoutdata())
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

        layoutData.MergeFrom(f);
    }
    return layoutData;
}

DiskLayout::DiskLayout(const ConfigProto& config)
{
    minPhysicalCylinder = minPhysicalHead = UINT_MAX;
    maxPhysicalCylinder = maxPhysicalHead = 0;

    numLogicalCylinders = config.layout().tracks();
    numLogicalHeads = config.layout().sides();

    groupSize = getTrackStep(config);
    headBias = config.drive().head_bias();
    swapSides = config.layout().swap_sides();

    switch (config.drive().drive_type())
    {
        case DRIVETYPE_APPLE2:
            headWidth = 4;
            break;

        default:
            headWidth = 1;
            break;
    }

    for (unsigned logicalCylinder = 0; logicalCylinder < numLogicalCylinders;
        logicalCylinder++)
        for (unsigned logicalHead = 0; logicalHead < numLogicalHeads;
            logicalHead++)
        {
            auto ltl = std::make_shared<LogicalTrackLayout>();
            CylinderHead ch(logicalCylinder, logicalHead);
            layoutByLogicalLocation[ch] = ltl;
            logicalLocations.push_back(ch);

            ltl->logicalCylinder = logicalCylinder;
            ltl->logicalHead = logicalHead;
            ltl->groupSize = groupSize;
            ltl->physicalCylinder =
                remapCylinderLogicalToPhysical(logicalCylinder);
            ltl->physicalHead = remapHeadLogicalToPhysical(logicalHead);

            minPhysicalCylinder =
                std::min(minPhysicalCylinder, ltl->physicalCylinder);
            maxPhysicalCylinder = std::max(maxPhysicalCylinder,
                ltl->physicalCylinder + ltl->groupSize - 1);
            minPhysicalHead = std::min(minPhysicalHead, ltl->physicalHead);
            maxPhysicalHead = std::max(maxPhysicalHead, ltl->physicalHead);

            auto layoutdata =
                getLayoutData(logicalCylinder, logicalHead, config);
            ltl->sectorSize = layoutdata.sector_size();
            ltl->diskSectorOrder = expandSectorList(layoutdata.physical());
            ltl->naturalSectorOrder = ltl->diskSectorOrder;
            std::sort(
                ltl->naturalSectorOrder.begin(), ltl->naturalSectorOrder.end());
            ltl->numSectors = ltl->naturalSectorOrder.size();

            if (layoutdata.has_filesystem())
            {
                ltl->filesystemSectorOrder =
                    expandSectorList(layoutdata.filesystem());
                if (ltl->filesystemSectorOrder.size() != ltl->numSectors)
                    error(
                        "filesystem sector order list doesn't contain the "
                        "right number of sectors");
            }
            else
                ltl->filesystemSectorOrder = ltl->naturalSectorOrder;

            for (int i = 0; i < ltl->numSectors; i++)
            {
                unsigned fid = ltl->naturalSectorOrder[i];
                unsigned lid = ltl->filesystemSectorOrder[i];
                ltl->sectorIdToNaturalOrdering[i] = fid;
                ltl->sectorIdToFilesystemOrdering[i] = fid;
            }
        };

    for (unsigned physicalCylinder = minPhysicalCylinder;
        physicalCylinder <= maxPhysicalCylinder;
        physicalCylinder++)
        for (unsigned physicalHead = minPhysicalHead;
            physicalHead <= maxPhysicalHead;
            physicalHead++)
        {
            auto ptl = std::make_shared<PhysicalTrackLayout>();
            CylinderHead ch(physicalCylinder, physicalHead);
            layoutByPhysicalLocation[ch] = ptl;
            physicalLocations.push_back(ch);

            ptl->physicalCylinder = physicalCylinder;
            ptl->physicalHead = physicalHead;
            ptl->groupOffset = (physicalCylinder - headBias) % groupSize;

            unsigned logicalCylinder =
                remapCylinderPhysicalToLogical(physicalCylinder);
            unsigned logicalHead = remapHeadPhysicalToLogical(physicalHead);
            ptl->logicalTrackLayout = findOrDefault(
                layoutByLogicalLocation, {logicalCylinder, logicalHead});
        }

    unsigned sectorOffset = 0;
    unsigned blockId = 0;
    for (auto& ch : getTrackOrdering(config.layout().filesystem_track_order(),
             numLogicalCylinders,
             numLogicalHeads))
    {
        const auto& ltl = layoutByLogicalLocation[ch];
        logicalLocationsInFilesystemOrder.push_back(ch);

        for (unsigned lid : ltl->filesystemSectorOrder)
        {
            LogicalLocation logicalLocation = {ch.cylinder, ch.head, lid};
            logicalSectorLocationBySectorOffset[sectorOffset] = logicalLocation;
            sectorOffsetByLogicalSectorLocation[logicalLocation] = sectorOffset;
            logicalSectorLocationsInFilesystemOrder.push_back(logicalLocation);
            sectorOffset += ltl->sectorSize;

            blockIdByLogicalSectorLocation[logicalLocation] = blockId;
            blockId++;
        }
    }
}

static ConfigProto createTestConfig(unsigned numCylinders,
    unsigned numHeads,
    unsigned numSectors,
    unsigned sectorSize)
{
    ConfigProto config;
    auto* layout = config.mutable_layout();
    layout->set_tracks(numCylinders);
    layout->set_sides(numHeads);
    auto* layoutData = layout->add_layoutdata();
    layoutData->set_sector_size(sectorSize);
    layoutData->mutable_physical()->set_count(numSectors);

    return config;
}

DiskLayout::DiskLayout(unsigned numCylinders,
    unsigned numHeads,
    unsigned numSectors,
    unsigned sectorSize):
    DiskLayout(createTestConfig(numCylinders, numHeads, numSectors, sectorSize))
{
}

static DiskLayout::LayoutBounds getBounds(std::ranges::view auto keys)
{
    DiskLayout::LayoutBounds r{.minCylinder = INT_MAX,
        .maxCylinder = INT_MIN,
        .minHead = INT_MAX,
        .maxHead = INT_MIN};

    for (const auto& ch : keys)
    {
        r.minCylinder = std::min<int>(r.minCylinder, ch.cylinder);
        r.maxCylinder = std::max<int>(r.maxCylinder, ch.cylinder);
        r.minHead = std::min<int>(r.minHead, ch.head);
        r.maxHead = std::max<int>(r.maxHead, ch.head);
    }

    return r;
}

DiskLayout::LayoutBounds DiskLayout::getPhysicalBounds() const
{
    return getBounds(std::views::keys(layoutByPhysicalLocation));
}

DiskLayout::LayoutBounds DiskLayout::getLogicalBounds() const
{
    return getBounds(std::views::keys(layoutByLogicalLocation));
}
