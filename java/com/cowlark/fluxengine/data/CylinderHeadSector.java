package com.cowlark.fluxengine.data;

/**
 * A cylinder/head/sector location, ported from lib/data/locations.h.
 */
public record CylinderHeadSector(int cylinder, int head, int sector) implements
        Comparable<CylinderHeadSector>
{
    @Override
    public int compareTo(CylinderHeadSector other)
    {
        int result = Integer.compare(cylinder, other.cylinder);
        if (result == 0)
            result = Integer.compare(head, other.head);
        if (result == 0)
            result = Integer.compare(sector, other.sector);
        return result;
    }

    public CylinderHead trackLocation()
    {
        return new CylinderHead(cylinder, head);
    }

    @Override
    public String toString()
    {
        return String.format("c%dh%ds%d", cylinder, head, sector);
    }
}