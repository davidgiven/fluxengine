package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import org.rob_smallshire.oaknut.discimage.Storage;
import java.io.IOException;
import java.util.Objects;

/* Delegate byte ranges directly to BlockDevice, which owns caching and pending changes.
 * Committing and reverting remain the filesystem adapter's responsibility. */
final class OaknutBlockDeviceStorage implements Storage
{
    private final BlockDevice device;
    private final int size;

    OaknutBlockDeviceStorage(BlockDevice device)
    {
        this.device = device;
        size = Math.multiplyExact(device.getBlockSize(), device.getBlockCount());
    }

    @Override
    public int size()
    {
        return size;
    }

    @Override
    public void read(int offset, byte[] destination, int destinationOffset, int length)
            throws IOException
    {
        Objects.checkFromIndexSize(offset, length, size);
        Objects.checkFromIndexSize(destinationOffset, length, destination.length);
        byte[] bytes = device.getBytes(offset, length).toByteArray();
        System.arraycopy(bytes, 0, destination, destinationOffset, length);
    }

    @Override
    public void write(int offset, byte[] source, int sourceOffset, int length) throws IOException
    {
        Objects.checkFromIndexSize(offset, length, size);
        Objects.checkFromIndexSize(sourceOffset, length, source.length);
        device.putBytes(offset, new Bytes(source).slice(sourceOffset, length));
    }
}
