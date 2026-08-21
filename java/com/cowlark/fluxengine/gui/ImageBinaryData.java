package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import org.exbin.auxiliary.binary_data.BinaryData;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.Map;

class ImageBinaryData implements BinaryData
{
    private final Disk disk;
    private final DiskLayout diskLayout;

    ImageBinaryData(Disk disk)
    {
        this.disk = disk;
        this.diskLayout = disk.diskLayout;
    }

    @Override
    public boolean isEmpty()
    {
        return false;
    }

    @Override
    public long getDataSize()
    {
        if (diskLayout == null)
            return 0;
        return disk.diskLayout.totalBytes;
    }

    @Override
    public byte getByte(long address)
    {
        if (diskLayout == null)
            return 0;
        Map.Entry<Long, LogicalLocation> entry =
                diskLayout.logicalSectorLocationBySectorOffset.floorEntry(address);
        Long sectorAddress = entry.getKey();
        long sectorOffset = address - sectorAddress;
        LogicalLocation logicalLocation = entry.getValue();
        long sectorSize = diskLayout.blockSizeByLogicalSectorLocation.get(logicalLocation);
        if (sectorOffset >= sectorSize)
            return 0;

        Sector sector = disk.image.get(logicalLocation);
        if (sector == null)
            return 0;
        if (sectorOffset >= sector.data.size())
            return 0;
        return (byte) sector.data.getByte((int) sectorOffset);
    }

    @Override
    public BinaryData copy()
    {
        return copy(0, getDataSize());
    }

    @Override
    public BinaryData copy(long l, long len)
    {
        byte[] bytes = new byte[(int) len];
        copyToArray(l, bytes, 0, (int) len);
        return null;
    }

    @Override
    public void copyToArray(long l, byte[] bytes, int i, int i1)
    {
        while (i1-- != 0)
            bytes[i++] = getByte(l++);
    }

    @Override
    public void saveToStream(OutputStream outputStream) throws IOException
    {
        throw new UnsupportedOperationException();

    }

    @Override
    public InputStream getDataInputStream()
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public void dispose()
    {
    }
}
