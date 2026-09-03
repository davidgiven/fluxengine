package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.FileSystemException;

public class ImageBlockDevice extends BlockDevice
{
    private final FilesystemOperation fso;
    public Image oldImage;
    public Image newImage = new Image();

    public ImageBlockDevice(FilesystemOperation fso)
    {
        super(fso.getDiskLayout());
        this.fso = fso;
        oldImage = fso.getImageReader().readImage();
    }

    @Override
    public Bytes getBlock(int blockNumber) throws IOException
    {
        LogicalLocation ll = getLocationOfBlock(blockNumber);
        Sector sector = newImage.get(ll);
        if (sector != null)
            return sector.data;
        sector = oldImage.get(ll);
        if (sector == null)
            throw new IOException("sector not found for block " + blockNumber);
        return sector.data;
    }

    @Override
    public void putBlock(int blockNumber, Bytes block) throws FileSystemException
    {
        LogicalLocation ll = getLocationOfBlock(blockNumber);
        newImage.put(ll).data = block;
    }

    @Override
    public void commit()
    {
        for (Sector sector : newImage)
            oldImage.put(sector.location).copyFrom(sector);
        newImage = new Image();
        try
        {
            fso.getImageWriter().writeImage(oldImage);
        } catch (Exception e)
        {
            // No writer configured or write failed; keep in-memory state
        }
    }

    @Override
    public void revert()
    {
        newImage = new Image();
    }

    @Override
    public boolean needsCommit()
    {
        return !newImage.empty();
    }
}
