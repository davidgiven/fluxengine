package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.util.Collection;

public abstract class TrackedBlockDevice extends BlockDevice
{
    protected Image originalData;
    protected Image changedData;

    public TrackedBlockDevice(
            DiskLayout diskLayout)
    {
        super(diskLayout);
        originalData = new Image();
        changedData = new Image();
    }

    @Override
    public Bytes getBlock(int blockNumber) throws IOException
    {
        CylinderHeadSector ll = getLocationOfBlock(blockNumber);
        if (changedData.contains(ll))
            return changedData.get(ll).data;
        if (originalData.contains(ll))
            return originalData.get(ll).data;
        populateTracks(originalData, ImmutableList.of(ll.trackLocation()));
        return originalData.get(ll).data;
    }

    @Override
    public void putBlock(int blockNumber, Bytes block) throws FileSystemException
    {
        CylinderHeadSector ll = getLocationOfBlock(blockNumber);
        changedData.put(ll).data = block;
    }

    protected abstract void commitTracks(Image source, ImmutableCollection<CylinderHead> lch);

    protected abstract void populateTracks(
            Image destination,
            ImmutableCollection<CylinderHead> lch);

    @Override
    public void commit()
    {
        ImmutableSet.Builder<CylinderHead> changedTracks = ImmutableSet.builder();
        for (CylinderHeadSector ll : changedData.getLogicalLocations())
        {
            CylinderHead lch = ll.trackLocation();
            copySectors(changedData, originalData, lch);
            changedTracks.add(lch);
        }
        commitTracks(changedData, changedTracks.build());
        changedData = new Image();
    }

    @Override
    public void revert()
    {
        changedData = new Image();
    }

    @Override
    public boolean needsCommit()
    {
        return !changedData.empty();
    }
}
