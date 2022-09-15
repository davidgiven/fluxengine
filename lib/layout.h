#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/flux.h"

class SectorListProto;

class Layout
{
public:
    Layout() {}

private:
    /* Can't copy. */
    Layout(const Layout&);
    Layout& operator=(const Layout&);

public:
    /* Translates logical track numbering (the numbers actually written in the
     * sector headers) to the track numbering on the actual drive, taking into
     * account tpi settings.
     */
    static unsigned remapTrackPhysicalToLogical(unsigned physicalTrack);
    static unsigned remapTrackLogicalToPhysical(unsigned logicalTrack);

    /* Translates logical side numbering (the numbers actually written in the
     * sector headers) to the sides used on the actual drive.
     */
    static unsigned remapSidePhysicalToLogical(unsigned physicalSide);
    static unsigned remapSideLogicalToPhysical(unsigned logicalSide);

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
    static std::shared_ptr<const Layout> getLayoutOfTrack(
        unsigned logicalTrack, unsigned logicalHead);

    /* Returns the layout of a given track via physical location. */
    static std::shared_ptr<const Layout> getLayoutOfTrackPhysical(
        unsigned physicalTrack, unsigned physicalSide);

    /* Expand a SectorList into the actual sector IDs. */
    static std::vector<unsigned> expandSectorList(
        const SectorListProto& sectorsProto);

public:
    unsigned numTracks;
    unsigned numSides;

    /* The number of sectors in this track. */
    unsigned numSectors;

    /* Physical location of this track. */
    unsigned physicalTrack;

    /* Physical side of this track. */
    unsigned physicalSide;

    /* Logical location of this track. */
    unsigned logicalTrack;

    /* Logical side of this track. */
    unsigned logicalSide;

    /* The number of physical tracks which need to be written for one logical
     * track. */
    unsigned groupSize;

    /* Number of bytes in a sector. */
    unsigned sectorSize;

    /* Sector IDs in disk order. */
    std::vector<unsigned> diskSectorOrder;

    /* Sector IDs in logical order. */
    std::vector<unsigned> logicalSectorOrder;

    /* Sector IDs in filesystem order. */
    std::vector<unsigned> filesystemSectorOrder;

    /* Mapping of filesystem order to logical order. */
    std::map<unsigned, unsigned> filesystemToLogicalSectorMap;

    /* Mapping of logical order to filesystem order. */
    std::map<unsigned, unsigned> logicalToFilesystemSectorMap;
};

#endif
