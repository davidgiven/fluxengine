#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/layout.pb.h"

class Layout
{
public:
    static std::vector<std::pair<int, int>> getTrackOrdering(
        unsigned guessedTracks = 0, unsigned guessedSides = 0);

    static const Layout& getLayoutOfTrack(
        unsigned logicalTrack, unsigned logicalHead);

public:
    unsigned numTracks;
    unsigned numSides;
    unsigned sectorSize;
    std::vector<unsigned> physicalSectors;
    std::vector<unsigned> logicalSectors;

    unsigned physicalSectorToLogical(unsigned sectorId);
    unsigned logicalSectorToPhysical(unsigned sectorId);
};

#endif
