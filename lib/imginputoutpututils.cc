#include "globals.h"
#include "lib/imagereader/imagereader.pb.h"
#include "lib/layout.pb.h"
#include "imginputoutpututils.h"

std::vector<std::pair<int, int>> getTrackOrdering(
    const LayoutProto& config, unsigned numTracks, unsigned numSides)
{
    int tracks = config.has_tracks() ? config.tracks() : numTracks;
    int sides = config.has_sides() ? config.sides() : numSides;

    std::vector<std::pair<int, int>> ordering;
    switch (config.order())
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
            Error() << "IMG: invalid track ordering";
    }

    return ordering;
}

LayoutProto::TrackdataProto getTrackFormat(
    const LayoutProto& config, unsigned track, unsigned side)
{
    LayoutProto::TrackdataProto trackdata;

    for (const auto& f : config.trackdata())
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

    return trackdata;
}

std::vector<unsigned> getTrackSectors(
    const LayoutProto::TrackdataProto& trackdata, unsigned numSectors)
{
    auto& physical = trackdata.physical();

    std::vector<unsigned> sectors;
    if (physical.has_count())
    {
        if (physical.sector_size() != 1)
            Error() << "if you use a sector count, you must specify exactly "
                       "one start sector";

        int startSector = physical.sector(0);
        for (int i = 0; i < physical.count(); i++)
            sectors.push_back(startSector + i);
    }
    else if (physical.guess_count())
    {
        if (physical.sector_size() != 1)
            Error() << "if you are guessing the number of sectors, you must "
                       "specify exactly one start sector";

        int startSector = physical.sector(0);
        for (int i = 0; i < numSectors; i++)
            sectors.push_back(startSector + i);
    }
    else if (trackdata.sector_size() > 0)
    {
        for (int sectorId : physical.sector())
            sectors.push_back(sectorId);
    }

    return sectors;
}
