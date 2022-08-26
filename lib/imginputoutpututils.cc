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

LayoutProto::TrackdataProto getTrackFormat(const LayoutProto& config,
    unsigned track,
    unsigned side)
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
    std::vector<unsigned> sectors;
    switch (trackdata.sectors_oneof_case())
    {
        case LayoutProto::TrackdataProto::SectorsOneofCase::kSectors:
        {
            for (int sectorId : trackdata.sectors().sector())
                sectors.push_back(sectorId);
            break;
        }

        case LayoutProto::TrackdataProto::SectorsOneofCase::
            kSectorRange:
        {
            int sectorId = trackdata.sector_range().start_sector();
			if (trackdata.sector_range().has_sector_count())
				numSectors = trackdata.sector_range().sector_count();
            for (int i = 0; i < numSectors; i++)
                sectors.push_back(sectorId + i);
            break;
        }

        default:
            break;
    }
    return sectors;
}
