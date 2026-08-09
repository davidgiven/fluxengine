package com.cowlark.fluxengine.data;

import com.google.common.collect.ArrayListMultimap;
import com.google.common.collect.ListMultimap;
import java.util.Set;
import java.util.TreeSet;

/**
 * A disk, being the result of reading a physical disk, ported from
 * lib/data/disk.h and lib/data/disk.cc.
 */
public class Disk
{
    public final ListMultimap<CylinderHead, Track> tracksByPhysicalLocation =
            ArrayListMultimap.create();
    public final ListMultimap<CylinderHead, Sector> sectorsByPhysicalLocation =
            ArrayListMultimap.create();
    public Image image = null;

    /* 0 if the period is unknown (e.g. if this Disk was made from an image). */
    public double rotationalPeriod = 0;

    public Disk()
    {
        image = new Image();
    }

    public Disk(Image image, DiskLayout diskLayout)
    {
        this.image = image;

        ListMultimap<CylinderHead, Sector> sectorsGroupedByTrack = ArrayListMultimap.create();
        for (Sector sector : image)
            sectorsGroupedByTrack.put(sector.physicalLocation, sector);

        Set<CylinderHead> sectorLocations = new TreeSet<>();
        for (CylinderHead ch : sectorsGroupedByTrack.keySet())
            sectorLocations.add(ch);

        for (CylinderHead physicalLocation : sectorLocations)
        {
            PhysicalTrackLayout ptl = diskLayout.layoutByPhysicalLocation.get(physicalLocation);
            LogicalTrackLayout ltl = ptl.logicalTrackLayout;

            Track decodedTrack = new Track();
            decodedTrack.ltl = ltl;
            decodedTrack.ptl = ptl;
            tracksByPhysicalLocation.put(physicalLocation, decodedTrack);

            for (Sector sector : sectorsGroupedByTrack.get(physicalLocation))
            {
                decodedTrack.allSectors.add(sector);
                decodedTrack.normalisedSectors.add(sector);
                sectorsByPhysicalLocation.put(physicalLocation, sector);
            }
        }
    }
}
