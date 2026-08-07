package com.cowlark.fluxengine.data;

import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;

/**
 * Summary information about a track, ported from lib/data/layout.h.
 */
public class TrackInfo
{
    public final int numCylinders;
    public final int numHeads;

    /* The number of sectors in this track. */
    public final int numSectors;

    /* Physical location of this track. */
    public final int physicalCylinder;

    /* Physical side of this track. */
    public final int physicalHead;

    /* Logical location of this track. */
    public final int logicalCylinder;

    /* Logical side of this track. */
    public final int logicalHead;

    /* The number of physical tracks which need to be written for one logical
     * track. */
    public final int groupSize;

    /* Number of bytes in a sector. */
    public final int sectorSize;

    /* Sector IDs in sector ID order. */
    public final ImmutableList<Integer> naturalSectorOrder;

    /* Sector IDs in disk order. */
    public final ImmutableList<Integer> diskSectorOrder;

    /* Sector IDs in filesystem order. */
    public final ImmutableList<Integer> filesystemSectorOrder;

    /* Mapping of filesystem order to natural order. */
    public final ImmutableMap<Integer, Integer> filesystemToNaturalSectorMap;

    /* Mapping of natural order to filesystem order. */
    public final ImmutableMap<Integer, Integer> naturalToFilesystemSectorMap;

    public TrackInfo(
        int numCylinders, int numHeads, int numSectors,
        int physicalCylinder, int physicalHead,
        int logicalCylinder, int logicalHead, int groupSize, int sectorSize,
        ImmutableList<Integer> naturalSectorOrder, ImmutableList<Integer> diskSectorOrder,
        ImmutableList<Integer> filesystemSectorOrder,
        ImmutableMap<Integer, Integer> filesystemToNaturalSectorMap,
        ImmutableMap<Integer, Integer> naturalToFilesystemSectorMap)
    {
        this.numCylinders = numCylinders;
        this.numHeads = numHeads;
        this.numSectors = numSectors;
        this.physicalCylinder = physicalCylinder;
        this.physicalHead = physicalHead;
        this.logicalCylinder = logicalCylinder;
        this.logicalHead = logicalHead;
        this.groupSize = groupSize;
        this.sectorSize = sectorSize;
        this.naturalSectorOrder = naturalSectorOrder;
        this.diskSectorOrder = diskSectorOrder;
        this.filesystemSectorOrder = filesystemSectorOrder;
        this.filesystemToNaturalSectorMap = filesystemToNaturalSectorMap;
        this.naturalToFilesystemSectorMap = naturalToFilesystemSectorMap;
    }
}