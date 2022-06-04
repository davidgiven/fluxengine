#include "lib/globals.h"
#include "lib/imagereader/imagereader.pb.h"
#include "lib/imginputoutpututils.h"

std::vector<std::pair<int, int>> getTrackOrdering(const ImgInputOutputProto& config,
	unsigned numTracks, unsigned numSides)
{
	int tracks = config.has_tracks() ? config.tracks() : numTracks;
	int sides = config.has_sides() ? config.sides() : numSides;

	std::vector<std::pair<int, int>> ordering;
	switch (config.order())
	{
		case ImgInputOutputProto::CHS:
		{
			for (int track = 0; track < tracks; track++)
			{
				for (int side = 0; side < sides; side++)
					ordering.push_back(std::make_pair(track, side));
			}
			break;
		}
	
		case ImgInputOutputProto::HCS:
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

void getTrackFormat(const ImgInputOutputProto& config,
		ImgInputOutputProto::TrackdataProto& trackdata, unsigned track, unsigned side)
{
	trackdata.Clear();
	for (const ImgInputOutputProto::TrackdataProto& f : config.trackdata())
	{
		if (f.has_track() && f.has_up_to_track() && ((track < f.track()) || (track > f.up_to_track())))
			continue;
		if (f.has_track() && !f.has_up_to_track() && (track != f.track()))
			continue;
		if (f.has_side() && (f.side() != side))
			continue;

		trackdata.MergeFrom(f);
	}
}

