package com.cowlark.fluxengine.data;

/**
 * The layout of a single physical track, ported from lib/data/layout.h.
 */
public class PhysicalTrackLayout
{
    /* Physical location of this track. */
    public final int physicalCylinder;

    /* Physical side of this track. */
    public final int physicalHead;

    /* Which member of the group this is. */
    public final int groupOffset;

    /* The logical track that this track is part of. */
    public final LogicalTrackLayout logicalTrackLayout;

    public PhysicalTrackLayout(
        int physicalCylinder, int physicalHead, int groupOffset,
        LogicalTrackLayout logicalTrackLayout)
    {
        this.physicalCylinder = physicalCylinder;
        this.physicalHead = physicalHead;
        this.groupOffset = groupOffset;
        this.logicalTrackLayout = logicalTrackLayout;
    }
}