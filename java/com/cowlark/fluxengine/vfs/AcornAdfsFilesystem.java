package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import org.rob_smallshire.oaknut.adfs.Adfs;
import org.rob_smallshire.oaknut.discimage.Storage;
import java.io.IOException;
import java.util.Objects;

public class AcornAdfsFilesystem extends Filesystem
{
    /* No filesystem operations are advertised until their adapters are implemented. */
    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of();

    private final AcornAdfsProto config;
    private final BlockDevice blockDevice;
    private Adfs adfs;

    public AcornAdfsFilesystem(AcornAdfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    /* Oaknut reads blocks on demand. BlockDevice supplies rollback, so oaknut snapshots are off. */
    private void mount() throws IOException
    {
        if (adfs != null)
            return;

        adfs = Adfs.fromStorage(new OaknutBlockDeviceStorage(blockDevice), false);
    }

    /* The storage is borrowed; closing oaknut does not commit or close the block device. */
    private void dismount() throws IOException
    {
        if (adfs == null)
            return;

        try
        {
            adfs.close();
        } finally
        {
            adfs = null;
        }
    }

    /* Lifecycle operations mirror the other filesystem adapters and invalidate the mounted handle. */
    @Override
    public boolean needsFlushing()
    {
        return blockDevice.needsCommit();
    }

    @Override
    public void flushChanges() throws IOException
    {
        blockDevice.commit();
        dismount();
    }

    @Override
    public void discardChanges() throws IOException
    {
        try
        {
            dismount();
        } finally
        {
            blockDevice.revert();
        }
    }

    @Override
    public void close() throws IOException
    {
        flushChanges();
    }

    /* Creation cannot mount an unformatted disk. All other data operations mount lazily. */
    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    @Override
    public void moveFile(VfsPath oldName, VfsPath newName) throws IOException
    {
        mount();
        throw new UnsupportedOperationException();
    }

    /* Delegate byte ranges directly to BlockDevice, which owns caching and pending changes.
     * Committing and reverting remain the filesystem adapter's responsibility. */
    static final class OaknutBlockDeviceStorage implements Storage
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
}
