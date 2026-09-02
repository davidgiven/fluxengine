package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import java.io.IOException;
import java.nio.file.FileSystemException;

public class ImageBlockDevice extends BlockDevice
{
    public ImageBlockDevice(FilesystemOperation fso)
    {
        super(fso.getDiskLayout());
    }

    @Override
    public Bytes getBlock(int blockNumber) throws IOException
    {
        return null;
    }

    @Override
    public void putBlock(int blockNumber, Bytes block) throws FileSystemException
    {

    }

    @Override
    public void commit()
    {

    }

    @Override
    public void revert()
    {

    }

    @Override
    public boolean needsCommit()
    {
        return false;
    }
}
