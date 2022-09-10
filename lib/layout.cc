#include "lib/globals.h"
#include "lib/layout.h"
#include "lib/proto.h"
#include "lib/environment.h"
#include <fmt/format.h>

static Local<std::map<std::pair<int, int>, std::unique_ptr<Layout>>>
    layoutCache;

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

static void expandSectors(const LayoutProto::SectorsProto& sectorsProto,
    std::vector<unsigned>& sectors)
{
    if (sectorsProto.has_count())
    {
        if (sectorsProto.sector_size() != 1)
            Error() << "LAYOUT: if you use a sector count, you must specify "
                       "exactly one start sector";

        int startSector = sectorsProto.sector(0);
        for (int i = 0; i < sectorsProto.count(); i++)
            sectors.push_back(startSector + i);
    }
    else if (sectorsProto.sector_size() > 0)
    {
        for (int sectorId : sectorsProto.sector())
            sectors.push_back(sectorId);
    }
    else
        Error() << "LAYOUT: no sectors in track!";
}

const Layout& Layout::getLayoutOfTrack(unsigned track, unsigned side)
{
    auto& layout = (*layoutCache)[std::make_pair(track, side)];
    if (!layout)
    {
        layout.reset(new Layout());

        LayoutProto::LayoutdataProto layoutdata;
        for (const auto& f : config.layout().layoutdata())
        {
            if (f.has_track() && f.has_up_to_track() &&
                ((track < f.track()) || (track > f.up_to_track())))
                continue;
            if (f.has_track() && !f.has_up_to_track() && (track != f.track()))
                continue;
            if (f.has_side() && (f.side() != side))
                continue;

            layoutdata.MergeFrom(f);
        }

        layout->numTracks = config.layout().tracks();
        layout->numSides = config.layout().sides();
        layout->sectorSize = layoutdata.sector_size();
        expandSectors(layoutdata.physical(), layout->physicalSectors);
        if (layoutdata.has_logical())
            expandSectors(layoutdata.logical(), layout->logicalSectors);
        else
            layout->logicalSectors = layout->physicalSectors;

        if (layout->logicalSectors.size() != layout->physicalSectors.size())
            Error() << fmt::format(
                "LAYOUT: physical and logical sectors lists are different "
                "sizes in {}.{}",
                track,
                side);
    }

    return *layout;
}

unsigned Layout::physicalSectorToLogical(unsigned physicalSectorId) const
{
    for (int i = 0; i < physicalSectors.size(); i++)
        if (physicalSectors[i] == physicalSectorId)
            return logicalSectors[i];
    Error() << fmt::format(
        "LAYOUT: physical sector {} not recognised", physicalSectorId);
    throw nullptr;
}

unsigned Layout::logicalSectorToPhysical(unsigned logicalSectorId) const
{
    for (int i = 0; i < logicalSectors.size(); i++)
        if (logicalSectors[i] == logicalSectorId)
            return physicalSectors[i];
    Error() << fmt::format(
        "LAYOUT: logical sector {} not recognised", logicalSectorId);
    throw nullptr;
}
