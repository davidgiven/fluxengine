#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/flux.h"

class SectorListProto;
class Track;

class Layout
{
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

    /* Uses the layout and current track and heads settings to determine
     * which Locations are going to be read from or written to. 8/
     */
    static std::vector<std::shared_ptr<const Track>> computeLocations();

    /* Returns a series of <track, side> pairs representing the filesystem
     * ordering of the disk, in logical numbers. */
    static std::vector<std::pair<int, int>> getTrackOrdering(
        unsigned guessedTracks = 0, unsigned guessedSides = 0);

    /* Returns the layout of a given track. */
    static std::shared_ptr<const Track> getLayoutOfTrack(
        unsigned logicalTrack, unsigned logicalHead);

    /* Returns the layout of a given track via physical location. */
    static std::shared_ptr<const Track> getLayoutOfTrackPhysical(
        unsigned physicalTrack, unsigned physicalSide);

    /* Expand a SectorList into the actual sector IDs. */
    static std::vector<unsigned> expandSectorList(
        const SectorListProto& sectorsProto);
};

class Track {
public:
	Track() {}

private:
    /* Can't copy. */
    Track(const Track&);
    Track& operator=(const Track&);

public:
    unsigned numTracks = 0;
    unsigned numSides = 0;

    /* The number of sectors in this track. */
    unsigned numSectors = 0;

    /* Physical location of this track. */
    unsigned physicalTrack = 0;

    /* Physical side of this track. */
    unsigned physicalSide = 0;

    /* Logical location of this track. */
    unsigned logicalTrack = 0;

    /* Logical side of this track. */
    unsigned logicalSide = 0;

    /* The number of physical tracks which need to be written for one logical
     * track. */
    unsigned groupSize = 0;

    /* Number of bytes in a sector. */
    unsigned sectorSize = 0;

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
