package com.cowlark.fluxengine.data;

import java.util.Comparator;

/**
 * A logical sector location, ported from lib/data/locations.h.
 */
public record LogicalLocation(int logicalCylinder, int logicalHead, int logicalSector) implements
        Comparable<LogicalLocation>
{
    private static final Comparator<LogicalLocation> COMPARATOR = Comparator
            .comparing(LogicalLocation::logicalCylinder)
            .thenComparingInt(LogicalLocation::logicalHead)
            .thenComparingInt(LogicalLocation::logicalSector);

    public CylinderHead trackLocation()
    {
        return new CylinderHead(logicalCylinder, logicalHead);
    }

    @Override
    public String toString()
    {
        return String.format("c%dh%ds%d", logicalCylinder, logicalHead, logicalSector);
    }

    @Override
    public int compareTo(LogicalLocation other)
    {
        return COMPARATOR.compare(this, other);
    }
}