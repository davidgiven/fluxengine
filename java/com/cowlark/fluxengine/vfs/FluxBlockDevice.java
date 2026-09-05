package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;

public class FluxBlockDevice extends TrackedBlockDevice
{
    private final FilesystemOperation fso;

    public FluxBlockDevice(FilesystemOperation fso)
    {
        super(fso.getDiskLayout());
        this.fso = fso;
    }

    @Override
    protected void commitTrack(Image source, CylinderHead lch)
    {
        LogicalTrackLayout ltl = diskLayout.layoutByLogicalLocation.get(lch);
        Image merged = new Image();
        for (int sectorId : ltl.diskSectorOrder)
        {
            CylinderHeadSector loc =
                    new CylinderHeadSector(ltl.logicalCylinder, ltl.logicalHead, sectorId);
            Sector sector = null;
            if (source.contains(loc))
                sector = source.get(loc);
            else if (originalData.contains(loc))
                sector = originalData.get(loc);

            if (sector != null)
                merged.put(loc).copyFrom(sector);
            else
            {
                Sector newSector = merged.put(loc);
                newSector.data = new com.cowlark.fluxengine.core.Bytes(ltl.sectorSize);
                newSector.status = Sector.Status.OK;
            }
        }

        Disk disk = new Disk();
        disk.image = merged;
        fso.writeDisk(disk, ImmutableList.of(lch));
    }

    @Override
    protected void populateTrack(Image destination, CylinderHead lch)
    {
        LogicalTrackLayout ltl = diskLayout.layoutByLogicalLocation.get(lch);
        ArrayList<Track> trackFluxes = new ArrayList<>();
        ArrayList<Sector> trackSectors = new ArrayList<>();
        fso.readAndDecodeTrack(ltl, trackFluxes, trackSectors);

        for (Sector sector : trackSectors)
            destination.put(sector.logicalLocation).copyFrom(sector);
    }
}
