package com.cowlark.fluxengine.data;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import lombok.EqualsAndHashCode;

/**
 * The layout of a single logical track, ported from lib/data/layout.h.
 */
@EqualsAndHashCode
public class LogicalTrackLayout
{
    /* Physical cylinder of the first element of the group. */
    public final int physicalCylinder;

    /* Physical head of the first element of the group. */
    public final int physicalHead;

    /* Size of this group. */
    public final int groupSize;

    /* Logical cylinder of this track. */
    public final int logicalCylinder;

    /* Logical side of this track. */
    public final int logicalHead;

    /* The number of sectors in this track. */
    public final int numSectors;

    /* Number of bytes in a sector. */
    public final int sectorSize;

    /* Sector IDs in sector ID order. */
    public final ImmutableList<Integer> naturalSectorOrder;

    /* Sector IDs in disk order. */
    public final ImmutableList<Integer> diskSectorOrder;

    /* Sector IDs in filesystem order. */
    public final ImmutableList<Integer> filesystemSectorOrder;

    /* Mapping of sector ID to filesystem ordering. */
    public final ImmutableMap<Integer, Integer> sectorIdToFilesystemOrdering;

    /* Mapping of sector ID to natural ordering. */
    public final ImmutableMap<Integer, Integer> sectorIdToNaturalOrdering;

    public LogicalTrackLayout(
            int physicalCylinder,
            int physicalHead,
            int groupSize,
            int logicalCylinder,
            int logicalHead,
            int numSectors,
            int sectorSize,
            ImmutableList<Integer> naturalSectorOrder,
            ImmutableList<Integer> diskSectorOrder,
            ImmutableList<Integer> filesystemSectorOrder,
            ImmutableMap<Integer, Integer> sectorIdToFilesystemOrdering,
            ImmutableMap<Integer, Integer> sectorIdToNaturalOrdering)
    {
        this.physicalCylinder = physicalCylinder;
        this.physicalHead = physicalHead;
        this.groupSize = groupSize;
        this.logicalCylinder = logicalCylinder;
        this.logicalHead = logicalHead;
        this.numSectors = numSectors;
        this.sectorSize = sectorSize;
        this.naturalSectorOrder = naturalSectorOrder;
        this.diskSectorOrder = diskSectorOrder;
        this.filesystemSectorOrder = filesystemSectorOrder;
        this.sectorIdToFilesystemOrdering = sectorIdToFilesystemOrdering;
        this.sectorIdToNaturalOrdering = sectorIdToNaturalOrdering;
    }
}