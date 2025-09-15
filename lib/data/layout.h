#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/data/flux.h"
#include "lib/config/layout.pb.h"
#include "lib/data/locations.h"

class SectorListProto;
class TrackInfo;

class Layout
{
public:
    /* Translates logical track numbering (the numbers actually written in the
     * sector headers) to the track numbering on the actual drive, taking into
     * account tpi settings.
     */
    static unsigned remapCylinderPhysicalToLogical(unsigned physicalCylinder);
    static unsigned remapCylinderLogicalToPhysical(unsigned logicalCylinder);

    /* Translates logical side numbering (the numbers actually written in the
     * sector headers) to the sides used on the actual drive.
     */
    static unsigned remapHeadPhysicalToLogical(unsigned physicalHead);
    static unsigned remapHeadLogicalToPhysical(unsigned logicalHead);

    /* Uses the layout and current track and heads settings to determine
     * which physical/logical tracks are going to be read from or written to.
     */
    static std::vector<CylinderHead> computePhysicalLocations();
    static std::vector<CylinderHead> computeLogicalLocations();

    /* Uses the current layout to compute the filesystem's block order, in
     * _logical_ tracks. */
    static std::vector<LogicalLocation> computeFilesystemLogicalOrdering();

    /* Given a list of CylinderHead locations, determines the minimum and
     * maximum track and side settings. */
    struct LayoutBounds
    {
        int minCylinder, maxCylinder, minHead, maxHead;
    };
    static LayoutBounds getBounds(const std::vector<CylinderHead>& locations);

    /* Returns a series of <track, side> pairs representing the filesystem
     * ordering of the disk, in logical numbers. */
    static std::vector<std::pair<int, int>> getTrackOrdering(
        LayoutProto::Order ordering,
        unsigned guessedCylinders = 0,
        unsigned guessedHeads = 0);

    /* Returns the layout of a given track. */
    static std::shared_ptr<const TrackInfo> getLayoutOfTrack(
        unsigned logicalCylinder, unsigned logicalHead);

    /* Returns the layout of a given track via physical location. */
    static std::shared_ptr<const TrackInfo> getLayoutOfTrackPhysical(
        unsigned physicalCylinder, unsigned physicalHead);

    /* Returns the layout of a given track via physical location. */
    static std::shared_ptr<const TrackInfo> getLayoutOfTrackPhysical(
        const CylinderHead& physicalLocation);

    /* Returns the layouts of a multiple tracks via physical location. */
    static std::vector<std::shared_ptr<const TrackInfo>>
    getLayoutOfTracksPhysical(const std::vector<CylinderHead>& locations);

    /* Expand a SectorList into the actual sector IDs. */
    static std::vector<unsigned> expandSectorList(
        const SectorListProto& sectorsProto);

    /* Return the head width of the current drive. */
    static int getHeadWidth();
};

class TrackInfo
{
public:
    TrackInfo() {}

private:
    /* Can't copy. */
    TrackInfo(const TrackInfo&);
    TrackInfo& operator=(const TrackInfo&);

public:
    unsigned numCylinders = 0;
    unsigned numHeads = 0;

    /* The number of sectors in this track. */
    unsigned numSectors = 0;

    /* Physical location of this track. */
    unsigned physicalCylinder = 0;

    /* Physical side of this track. */
    unsigned physicalHead = 0;

    /* Logical location of this track. */
    unsigned logicalCylinder = 0;

    /* Logical side of this track. */
    unsigned logicalHead = 0;

    /* The number of physical tracks which need to be written for one logical
     * track. */
    unsigned groupSize = 0;

    /* Number of bytes in a sector. */
    unsigned sectorSize = 0;

    /* Sector IDs in sector ID order. This is the order in which the appear in
     * disk images. */
    std::vector<unsigned> naturalSectorOrder;

    /* Sector IDs in disk order. This is the order they are written to the disk.
     */
    std::vector<unsigned> diskSectorOrder;

    /* Sector IDs in filesystem order. This is the order in which the filesystem
     * uses them. */
    std::vector<unsigned> filesystemSectorOrder;

    /* Mapping of filesystem order to natural order. */
    std::map<unsigned, unsigned> filesystemToNaturalSectorMap;

    /* Mapping of natural order to filesystem order. */
    std::map<unsigned, unsigned> naturalToFilesystemSectorMap;
};

#endif
