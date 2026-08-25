package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalLocation;
import java.io.IOException;
import java.nio.file.FileSystemException;

public abstract class TrackedBlockDevice extends BlockDevice
{
    private Image originalData;
    private Image changedData;

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
        LogicalLocation ll = getLocationOfBlock(blockNumber);
        if (changedData.contains(ll))
            return changedData.get(ll).data;
        if (originalData.contains(ll))
            return originalData.get(ll).data;
        populateTrack(originalData, ll.trackLocation());
        return originalData.get(ll).data;
    }

    @Override
    public void putBlock(int blockNumber, Bytes block) throws FileSystemException
    {
        LogicalLocation ll = getLocationOfBlock(blockNumber);
        changedData.put(ll).data = block;
    }

    protected abstract void commitTrack(Image source, CylinderHead lch);

    protected abstract void populateTrack(Image destination, CylinderHead lch);

    @Override
    public void commit()
    {
        changedData.getLogicalLocations().stream().map(ll -> ll.trackLocation()).forEach(lch -> {
            commitTrack(changedData, lch);
            copySectors(changedData, originalData, lch);
        });
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
