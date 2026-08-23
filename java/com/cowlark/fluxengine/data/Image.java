package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.core.Bytes;
import java.util.Collection;
import java.util.Iterator;
import java.util.LinkedHashMap;
import java.util.Map;
import java.util.stream.Stream;

/**
 * A disk image, a collection of sectors indexed by logical location, ported
 * from lib/data/image.h.
 */
public class Image implements Iterable<Sector>
{
    private final Map<LogicalLocation, Sector> sectors = new LinkedHashMap<>();
    private Geometry geometry = new Geometry();

    public Image()
    {
    }

    public Image(Collection<Sector> sectors)
    {
        for (Sector sector : sectors)
            this.sectors.put(sector.location, sector);
        calculateSize();
    }

    public void calculateSize()
    {
        geometry = new Geometry();
        int maxSector = 0;
        for (Map.Entry<LogicalLocation, Sector> entry : sectors.entrySet())
        {
            Sector sector = entry.getValue();
            if (sector != null)
            {
                geometry.numCylinders =
                        Math.max(geometry.numCylinders, sector.location.logicalCylinder() + 1);
                geometry.numHeads = Math.max(geometry.numHeads, sector.location.logicalHead() + 1);
                geometry.firstSector =
                        Math.min(geometry.firstSector, sector.location.logicalSector());
                maxSector = Math.max(maxSector, sector.location.logicalSector());
                geometry.sectorSize = Math.max(geometry.sectorSize, sector.data.size());
                geometry.totalBytes += geometry.sectorSize;
            }
        }
        geometry.numSectors = maxSector - geometry.firstSector + 1;
    }

    public void clear()
    {
        sectors.clear();
        geometry = new Geometry();
    }

    public boolean empty()
    {
        return sectors.isEmpty();
    }

    public boolean contains(LogicalLocation location)
    {
        return sectors.containsKey(location);
    }

    public boolean contains(int cylinder, int head, int sector)
    {
        return contains(new LogicalLocation(cylinder, head, sector));
    }

    public Sector get(LogicalLocation location)
    {
        return sectors.get(location);
    }

    public Sector get(int cylinder, int head, int sector)
    {
        return get(new LogicalLocation(cylinder, head, sector));
    }

    public Sector put(LogicalLocation location)
    {
        Sector sector = new Sector(location);
        sectors.put(location, sector);
        return sector;
    }

    public Sector put(int cylinder, int head, int sector)
    {
        return put(new LogicalLocation(cylinder, head, sector));
    }

    public void erase(LogicalLocation location)
    {
        sectors.remove(location);
    }

    public void erase(int cylinder, int head, int sector)
    {
        erase(new LogicalLocation(cylinder, head, sector));
    }

    public void addMissingSectors(DiskLayout layout, boolean populated)
    {
        for (LogicalLocation location : layout.logicalSectorLocationsInFilesystemOrder)
        {
            if (!sectors.containsKey(location))
            {
                LogicalTrackLayout ltl =
                        layout.layoutByLogicalLocation.get(location.trackLocation());
                Sector sector = new Sector(location);

                if (populated)
                    sector.data = new Bytes(ltl.sectorSize);
                else
                    sector.status = Sector.Status.MISSING;

                sectors.put(location, sector);
            }
        }
        calculateSize();
    }

    public void populateSectorPhysicalLocationsFromLogicalLocations(DiskLayout diskLayout)
    {
        Image tempImage = new Image();
        for (Sector sector : this)
        {
            LogicalTrackLayout ltl =
                    diskLayout.layoutByLogicalLocation.get(sector.location.trackLocation());
            Sector newSector = tempImage.put(
                    sector.location.logicalCylinder(),
                    sector.location.logicalHead(),
                    sector.location.logicalSector());
            newSector.location = sector.location;
            newSector.status = sector.status;
            newSector.position = sector.position;
            newSector.clockNs = sector.clockNs;
            newSector.headerStartTimeNs = sector.headerStartTimeNs;
            newSector.headerEndTimeNs = sector.headerEndTimeNs;
            newSector.dataStartTimeNs = sector.dataStartTimeNs;
            newSector.dataEndTimeNs = sector.dataEndTimeNs;
            newSector.data = sector.data;
            newSector.records = sector.records;
            newSector.physicalLocation = new CylinderHead(ltl.physicalCylinder, ltl.physicalHead);
        }

        for (Sector sector : tempImage)
            sectors.put(sector.location, sector);
    }

    public Geometry getGeometry()
    {
        return geometry;
    }

    public void setGeometry(Geometry geometry)
    {
        this.geometry = geometry;
    }

    @Override
    public Iterator<Sector> iterator()
    {
        return sectors.values().iterator();
    }

    public Stream<Sector> stream()
    {
        return sectors.values().stream();
    }
}