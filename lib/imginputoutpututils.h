#ifndef IMGINPUTOUTPUTUTILS_H
#define IMGINPUTOUTPUTUTILS_H

extern std::vector<std::pair<int, int>> getTrackOrdering(
    const LayoutProto& config,
    unsigned numTracks = 0,
    unsigned numSides = 0);

extern LayoutProto::TrackdataProto getTrackFormat(const LayoutProto& config,
    unsigned track,
    unsigned side);

extern std::vector<unsigned> getTrackSectors(
    const LayoutProto::TrackdataProto& trackdata, unsigned numSectors = 0);

#endif
