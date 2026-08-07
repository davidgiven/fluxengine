package com.cowlark.fluxengine.data;

/**
 * A logical sector location, ported from lib/data/locations.h.
 */
public record LogicalLocation(int logicalCylinder, int logicalHead, int logicalSector)
{
    public CylinderHead trackLocation()
    {
        return new CylinderHead(logicalCylinder, logicalHead);
    }

    @Override
    public String toString()
    {
        return String.format("c%dh%ds%d", logicalCylinder, logicalHead, logicalSector);
    }
}