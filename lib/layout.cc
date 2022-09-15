#include "lib/globals.h"
#include "lib/layout.h"
#include "lib/proto.h"
#include "lib/environment.h"
#include <fmt/format.h>

static Local<std::map<std::pair<int, int>, std::shared_ptr<Layout>>>
    layoutCache;

static unsigned getTrackStep()
{
    unsigned track_step =
        (config.tpi() == 0) ? 1 : (config.drive().tpi() / config.tpi());

    if (track_step == 0)
        Error()
            << "this drive can't write this image, because the head is too big";
    return track_step;
}

unsigned Layout::remapTrackPhysicalToLogical(unsigned ptrack)
{
    return (ptrack - config.drive().head_bias()) / getTrackStep();
}

unsigned Layout::remapTrackLogicalToPhysical(unsigned ltrack)
{
    return config.drive().head_bias() + ltrack * getTrackStep();
}

unsigned Layout::remapSidePhysicalToLogical(unsigned pside)
{
    return pside ^ config.layout().swap_sides();
}

unsigned Layout::remapSideLogicalToPhysical(unsigned lside)
{
    return lside ^ config.layout().swap_sides();
}

std::vector<std::shared_ptr<const Layout>> Layout::computeLocations()
{
    std::set<unsigned> tracks;
    if (config.has_tracks())
        tracks = iterate(config.tracks());
    else
        tracks = iterate(0, config.layout().tracks());

    std::set<unsigned> heads;
    if (config.has_heads())
        heads = iterate(config.heads());
    else
        heads = iterate(0, config.layout().sides());

    std::vector<std::shared_ptr<const Layout>> locations;
    for (unsigned logicalTrack : tracks)
    {
        for (unsigned logicalHead : heads)
            locations.push_back(getLayoutOfTrack(logicalTrack, logicalHead));
    }
    return locations;
}

std::vector<std::pair<int, int>> Layout::getTrackOrdering(
    unsigned guessedTracks, unsigned guessedSides)
{
    auto layout = config.layout();
    int tracks = layout.has_tracks() ? layout.tracks() : guessedTracks;
    int sides = layout.has_sides() ? layout.sides() : guessedSides;

    std::vector<std::pair<int, int>> ordering;
    switch (layout.order())
    {
        case LayoutProto::CHS:
        {
            for (int track = 0; track < tracks; track++)
            {
                for (int side = 0; side < sides; side++)
                    ordering.push_back(std::make_pair(track, side));
            }
            break;
        }

        case LayoutProto::HCS:
        {
            for (int side = 0; side < sides; side++)
            {
                for (int track = 0; track < tracks; track++)
                    ordering.push_back(std::make_pair(track, side));
            }
            break;
        }

        default:
            Error() << "LAYOUT: invalid track ordering";
    }

    return ordering;
}

std::vector<unsigned> Layout::expandSectorList(
    const SectorListProto& sectorsProto)
{
    std::vector<unsigned> sectors;

    if (sectorsProto.has_count())
    {
        if (sectorsProto.sector_size() != 0)
            Error() << "LAYOUT: if you use a sector count, you can't use an "
                       "explicit sector list";

        for (int i = 0; i < sectorsProto.count(); i++)
            sectors.push_back(
                sectorsProto.start_sector() +
                ((i * sectorsProto.skew()) % sectorsProto.count()));
    }
    else if (sectorsProto.sector_size() > 0)
    {
        for (int sectorId : sectorsProto.sector())
            sectors.push_back(sectorId);
    }
    else
        Error() << "LAYOUT: no sectors in sector definition!";

    return sectors;
}

std::shared_ptr<const Layout> Layout::getLayoutOfTrack(
    unsigned logicalTrack, unsigned logicalSide)
{
    auto& layout = (*layoutCache)[std::make_pair(logicalTrack, logicalSide)];
    if (!layout)
    {
    	layout = std::make_shared<Layout>();

        LayoutProto::LayoutdataProto layoutdata;
        for (const auto& f : config.layout().layoutdata())
        {
            if (f.has_track() && f.has_up_to_track() &&
                ((logicalTrack < f.track()) ||
                    (logicalTrack > f.up_to_track())))
                continue;
            if (f.has_track() && !f.has_up_to_track() &&
                (logicalTrack != f.track()))
                continue;
            if (f.has_side() && (f.side() != logicalSide))
                continue;

            layoutdata.MergeFrom(f);
        }

        layout->numTracks = config.layout().tracks();
        layout->numSides = config.layout().sides();
        layout->sectorSize = layoutdata.sector_size();
        layout->logicalTrack = logicalTrack;
        layout->logicalSide = logicalSide;
        layout->physicalTrack = remapTrackLogicalToPhysical(logicalTrack);
        layout->physicalSide = logicalSide ^ config.layout().swap_sides();
        layout->groupSize = getTrackStep();
        layout->diskSectorOrder = expandSectorList(layoutdata.physical());
        layout->logicalSectorOrder = layout->diskSectorOrder;
        std::sort(
            layout->diskSectorOrder.begin(), layout->diskSectorOrder.end());
        layout->numSectors = layout->logicalSectorOrder.size();

        if (layoutdata.has_filesystem())
        {
            layout->filesystemSectorOrder =
                expandSectorList(layoutdata.filesystem());
            if (layout->filesystemSectorOrder.size() != layout->numSectors)
                Error()
                    << "filesystem sector order list doesn't contain the right "
                       "number of sectors";
        }
        else
        {
            for (unsigned sectorId : layout->logicalSectorOrder)
                layout->filesystemSectorOrder.push_back(sectorId);
        }

        for (int i = 0; i < layout->numSectors; i++)
        {
            unsigned f = layout->logicalSectorOrder[i];
            unsigned l = layout->filesystemSectorOrder[i];
            layout->filesystemToLogicalSectorMap[f] = l;
            layout->logicalToFilesystemSectorMap[l] = f;
        }
    }

    return layout;
}

std::shared_ptr<const Layout> Layout::getLayoutOfTrackPhysical(
    unsigned physicalTrack, unsigned physicalSide)
{
    return getLayoutOfTrack(remapTrackPhysicalToLogical(physicalTrack),
        remapSidePhysicalToLogical(physicalSide));
}
