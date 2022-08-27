#include "lib/globals.h"
#include "lib/layout.h"
#include "lib/proto.h"

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

LayoutProto::LayoutdataProto Layout::getLayoutOfTrack(
    unsigned track, unsigned side)
{
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

    return layoutdata;
}

std::vector<unsigned> Layout::getSectorsInTrack(
    const LayoutProto::LayoutdataProto& trackdata, unsigned guessedSectors)
{
    auto& physical = trackdata.physical();

    std::vector<unsigned> sectors;
    if (physical.has_count())
    {
        if (physical.sector_size() != 1)
            Error() << "LAYOUT: if you use a sector count, you must specify "
                       "exactly one start sector";

        int startSector = physical.sector(0);
        for (int i = 0; i < physical.count(); i++)
            sectors.push_back(startSector + i);
    }
    else if (physical.guess_count())
    {
        if (physical.sector_size() != 1)
            Error() << "LAYOUT: if you are guessing the number of sectors, you "
                       "must specify exactly one start sector";

        int startSector = physical.sector(0);
        for (int i = 0; i < guessedSectors; i++)
            sectors.push_back(startSector + i);
    }
    else if (trackdata.sector_size() > 0)
    {
        for (int sectorId : physical.sector())
            sectors.push_back(sectorId);
    }

    return sectors;
}

std::vector<unsigned> Layout::getLogicalSectorsInTrack(
    const LayoutProto::LayoutdataProto& trackdata)
{
    auto& logical = trackdata.logical();

    if (logical.sector_size() == 0)
        return getSectorsInTrack(trackdata);

    std::vector<unsigned> sectors;
    if (logical.has_count())
    {
        if (logical.sector_size() != 1)
            Error() << "LAYOUT: if you use a sector count, you must specify "
                       "exactly one start sector";

        int startSector = logical.sector(0);
        for (int i = 0; i < logical.count(); i++)
            sectors.push_back(startSector + i);
    }
    else
    {
        for (int sectorId : logical.sector())
            sectors.push_back(sectorId);
    }

    return sectors;
}

std::map<unsigned, unsigned> Layout::getLogicalToPhysicalMap(
    const LayoutProto::LayoutdataProto& trackdata)
{
    auto physical = getSectorsInTrack(trackdata);
    auto logical = getSectorsInTrack(trackdata);
    if (physical.size() != logical.size())
        Error() << "LAYOUT: physical sector list and logical sector list have "
                   "different sizes";
	
	std::map<unsigned, unsigned> map;
	for (int i = 0; i < physical.size(); i++)
		map[logical[i]] = physical[i];
	return map;
}
