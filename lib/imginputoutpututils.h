#ifndef IMGINPUTOUTPUTUTILS_H
#define IMGINPUTOUTPUTUTILS_H

extern std::vector<std::pair<int, int>> getTrackOrdering(const ImgInputOutputProto& config,
		unsigned numTracks = 0, unsigned numSides = 0);

extern void getTrackFormat(const ImgInputOutputProto& config,
		ImgInputOutputProto::TrackdataProto& trackdata, unsigned track, unsigned side);

#endif


