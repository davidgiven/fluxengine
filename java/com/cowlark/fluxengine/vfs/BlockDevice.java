package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.FileSystemException;

/**
 * Represents a block device, capable of reading and writing individual blocks on a device.
 * Block numbers are contiguous and start at 0.
 */
public abstract class BlockDevice implements AutoCloseable
{
    protected final DiskLayout diskLayout;
    protected int blockSize;

    public BlockDevice(DiskLayout diskLayout)
    {
        blockSize = diskLayout.blockSizeByLogicalSectorLocation
                .values()
                .stream()
                .max(Long::compareTo)
                .get()
                .intValue();
        this.diskLayout = diskLayout;
    }

    protected static void copySectors(Image source, Image destination, CylinderHead lch)
    {
        for (Sector sector : source)
            if (sector.location.trackLocation().equals(lch))
                destination.put(sector.location).data = sector.data;
    }

    public int getBlockSize()
    {
        return blockSize;
    }

    public int getBlockCount()
    {
        return diskLayout.blockIdByLogicalSectorLocation.size();
    }

    public abstract Bytes getBlock(int blockNumber) throws IOException;

    public abstract void putBlock(int blockNumber, Bytes block) throws FileSystemException;

    public abstract void commit();

    public abstract void revert();

    public abstract boolean needsCommit();

    @Override
    public void close()
    {
    }

    protected LogicalLocation getLocationOfBlock(int blockNumber) throws FileSystemException
    {
        try
        {
            return diskLayout.logicalSectorLocationsInFilesystemOrder.get(blockNumber);
        } catch (IndexOutOfBoundsException e)
        {
            throw new FileSystemException(String.format(
                    "block number %d out of bounds",
                    blockNumber));
        }
    }

    public Bytes getBlocks(int blockNumber, int blockCount) throws IOException
    {
        Bytes output = new Bytes();
        ByteWriter bw = new ByteWriter(output);

        while (blockCount-- != 0)
            bw.write(getBlock(blockNumber++));
        return output;
    }

    public void putBlocks(int blockNumber, Bytes bytes) throws FileSystemException
    {
        ByteReader br = new ByteReader(bytes);

        while (!br.eof())
        {
            Bytes block = br.read(getBlockSize());
            putBlock(blockNumber++, block);
        }
    }

    public Bytes getBytes(int offset, int length) throws IOException
    {
        int blockSize = getBlockSize();
        Bytes result = new Bytes();
        ByteWriter bw = new ByteWriter(result);

        while (length != 0)
        {
            int blockNumber = offset / blockSize;
            int blockOffset = offset % blockSize;
            int blockLen = Math.min(blockOffset + length, blockSize) - blockOffset;
            Bytes block = getBlock(blockNumber);

            bw.write(block.slice(blockOffset, blockLen));
            length -= blockLen;
            offset += blockLen;
        }

        return result;
    }

    public void putBytes(int offset, Bytes data) throws IOException
    {
        int blockSize = getBlockSize();
        int length = data.size();
        ByteReader br = new ByteReader(data);

        while (length != 0)
        {
            int blockNumber = offset / blockSize;
            int blockOffset = offset % blockSize;
            int blockLen = Math.min(blockOffset + length, blockSize) - blockOffset;

            if ((blockOffset != 0) || (blockLen != blockSize))
            {
                /* Partial block; do a read-then-modify */

                Bytes block = getBlock(blockNumber);
                block.writer().seek(blockOffset).write(br.read(blockLen));
                putBlock(blockNumber, block);
            } else
            {
                /* Complete block; just write */

                putBlock(blockNumber, br.read(blockLen));
            }

            length -= blockLen;
            offset += blockLen;
        }
    }

}
