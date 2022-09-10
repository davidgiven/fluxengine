#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/flux.h"

class Layout
{
public:
    Layout() {}

private:
    /* Can't copy. */
    Layout(const Layout&);
    Layout& operator=(const Layout&);

public:
    /* Translates logical track numbering (filesystem numbering) to
     * the track numbering on the actual drive, taking into account tpi
     * settings.
     */
    static unsigned remapTrackPhysicalToLogical(unsigned physicalTrack);
    static unsigned remapTrackLogicalToPhysical(unsigned logicalTrack);

    /* Computes a Location for a given logical track and side, which
     * contains the physical drive location and group size. */
    static Location computeLocationFor(
        unsigned logicalTrack, unsigned logicalSide);

    /* Uses the layout and current track and heads settings to determine
     * which Locations are going to be read from or written to. 8/
     */
    static std::set<Location> computeLocations();

    /* Returns a series of <track, side> pairs representing the filesystem
     * ordering of the disk, in logical numbers. */
    static std::vector<std::pair<int, int>> getTrackOrdering(
        unsigned guessedTracks = 0, unsigned guessedSides = 0);

    /* Returns the layout of a given track. */
    static const Layout& getLayoutOfTrack(
        unsigned logicalTrack, unsigned logicalHead);

public:
    unsigned numTracks;
    unsigned numSides;
    unsigned numSectors;
    unsigned sectorSize;

    /* Physical sector IDs in disk order. */
    std::vector<unsigned> physicalSectors;

    /* Logical sector IDs in disk order. */
    std::vector<unsigned> logicalSectorsOnDisk;

    /* Logical sector IDs in filesystem order. */
    std::vector<unsigned> logicalSectors;

    unsigned physicalSectorToLogical(unsigned sectorId) const;
    unsigned logicalSectorToPhysical(unsigned sectorId) const;
};

#endif
