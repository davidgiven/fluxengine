#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/layout.pb.h"

class Layout
{
public:
    static std::vector<std::pair<int, int>> getTrackOrdering(
        unsigned guessedTracks = 0, unsigned guessedSides = 0);

    static LayoutProto::LayoutdataProto getLayoutOfTrack(
        unsigned logicalTrack, unsigned logicalHead);

    static std::vector<unsigned> getSectorsInTrack(
        const LayoutProto::LayoutdataProto& trackdata,
        unsigned guessedSectors = 0);
};

#endif
