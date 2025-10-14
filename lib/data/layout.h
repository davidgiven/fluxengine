#ifndef LAYOUT_H
#define LAYOUT_H

#include "lib/data/disk.h"
#include "lib/config/layout.pb.h"
#include "lib/config/config.h"
#include "lib/data/locations.h"

class ConfigProto;

struct LogicalTrackLayout
{
    /* Physical cylinder of the first element of the group. */
    unsigned physicalCylinder;

    /* Physical head of the first element of the group. */
    unsigned physicalHead;

    /* Size of this group. */
    unsigned groupSize;

    /* Logical cylinder of this track. */
    unsigned logicalCylinder = 0;

    /* Logical side of this track. */
    unsigned logicalHead = 0;

    /* The number of sectors in this track. */
    unsigned numSectors = 0;

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

    /* Mapping of sector ID to filesystem ordering. */
    std::map<unsigned, unsigned> sectorIdToFilesystemOrdering;

    /* Mapping of sector ID to natural ordering. */
    std::map<unsigned, unsigned> sectorIdToNaturalOrdering;
};

struct PhysicalTrackLayout
{
    /* Physical location of this track. */
    unsigned physicalCylinder;

    /* Physical side of this track. */
    unsigned physicalHead;

    /* Which member of the group this is. */
    unsigned groupOffset;

    /* The logical track that this track is part of. */
    std::shared_ptr<const LogicalTrackLayout> logicalTrackLayout;
};

class DiskLayout
{
public:
    DiskLayout(const ConfigProto& config = globalConfig());

    /* Makes a simplified layout for testing. */

    DiskLayout(unsigned numCylinders,
        unsigned numHeads,
        unsigned numSectors,
        unsigned sectorSize);

public:
    /* Logical size. */

    unsigned numLogicalCylinders;
    unsigned numLogicalHeads;

    /* Physical size and properties. */

    unsigned minPhysicalCylinder, maxPhysicalCylinder;
    unsigned minPhysicalHead, maxPhysicalHead;
    unsigned groupSize;  /* Number of physical cylinders per logical cylinder */
    unsigned headBias;   /* Physical cylinder offset */
    unsigned headWidth;  /* Width of the physical head */
    bool swapSides;      /* Whether sides need to be swapped */
    unsigned totalBytes; /* Total number of bytes on the disk. */

    /* Physical and logical layouts by location. */

    std::map<CylinderHead, std::shared_ptr<const PhysicalTrackLayout>>
        layoutByPhysicalLocation;
    std::map<CylinderHead, std::shared_ptr<const LogicalTrackLayout>>
        layoutByLogicalLocation;

    /* Ordered lists of physical and logical locations. */

    std::vector<CylinderHead> logicalLocations;
    std::vector<CylinderHead> logicalLocationsInFilesystemOrder;
    std::vector<CylinderHead> physicalLocations;

    /* Ordered lists of sector locations, plus the reverse mapping. */

    std::vector<LogicalLocation> logicalSectorLocationsInFilesystemOrder;
    std::map<LogicalLocation, unsigned> blockIdByLogicalSectorLocation;
    std::vector<CylinderHeadSector> physicalSectorLocationsInFilesystemOrder;

    /* Mapping from logical location to sector offset and back again. */

    std::map<unsigned, LogicalLocation> logicalSectorLocationBySectorOffset;
    std::map<LogicalLocation, unsigned> sectorOffsetByLogicalSectorLocation;

public:
    unsigned remapCylinderPhysicalToLogical(unsigned physicalCylinder) const
    {
        return (physicalCylinder - headBias) / groupSize;
    }

    unsigned remapCylinderLogicalToPhysical(unsigned logicalCylinder) const
    {
        return headBias + logicalCylinder * groupSize;
    }

    unsigned remapHeadPhysicalToLogical(unsigned physicalHead) const
    {
        return physicalHead ^ swapSides;
    }

    unsigned remapHeadLogicalToPhysical(unsigned logicalHead) const
    {
        return logicalHead ^ swapSides;
    }

    /* Given a list of CylinderHead locations, determines the minimum and
     * maximum track and side settings. */
    struct LayoutBounds
    {
        std::strong_ordering operator<=>(
            const LayoutBounds& other) const = default;

        int minCylinder, maxCylinder, minHead, maxHead;
    };

    LayoutBounds getPhysicalBounds() const;
    LayoutBounds getLogicalBounds() const;
};

static std::shared_ptr<DiskLayout> createDiskLayout(
    const ConfigProto& config = globalConfig())
{
    return std::make_shared<DiskLayout>(config);
}

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
