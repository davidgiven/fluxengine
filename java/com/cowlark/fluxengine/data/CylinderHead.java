package com.cowlark.fluxengine.data;

/**
 * A cylinder/head location, ported from lib/data/locations.h.
 */
public record CylinderHead(int cylinder, int head) implements Comparable<CylinderHead>
{
    @Override
    public int compareTo(CylinderHead other)
    {
        int result = Integer.compare(cylinder, other.cylinder);
        if (result == 0)
            result = Integer.compare(head, other.head);
        return result;
    }
}
