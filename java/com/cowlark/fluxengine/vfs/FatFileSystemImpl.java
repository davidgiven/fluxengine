package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableSet;
import de.waldheinz.fs.ReadOnlyException;
import de.waldheinz.fs.fat.FatType;
import de.waldheinz.fs.fat.SuperFloppyFormatter;
import java.io.IOException;
import java.nio.ByteBuffer;

public class FatFileSystemImpl extends FileSystemImpl
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(Capability.OP_CREATE);

    private final FatFsProto config;
    private final BlockDevice blockDevice;
    private final de.waldheinz.fs.BlockDevice fatDevice;

    private static class BlockDeviceAdapter implements de.waldheinz.fs.BlockDevice
    {
        private final BlockDevice underlying;

        BlockDeviceAdapter(BlockDevice underlying)
        {
            this.underlying = underlying;
        }

        @Override
        public long getSize() throws IOException
        {
            return underlying.getBlockCount() * underlying.getBlockSize();
        }

        @Override
        public void read(long l, ByteBuffer byteBuffer) throws IOException
        {
            int blockNumber = (int) l / underlying.getBlockSize();
            Bytes bytes = underlying.getBlock(blockNumber);
            byteBuffer.clear().put(bytes.toByteArray());
        }

        @Override
        public void write(long l, ByteBuffer byteBuffer)
                throws ReadOnlyException, IOException, IllegalArgumentException
        {
            int blockNumber = (int) l / underlying.getBlockSize();
            Bytes bytes = new Bytes(byteBuffer.array());
            underlying.putBlock(blockNumber, bytes);
        }

        @Override
        public void flush() throws IOException
        {

        }

        @Override
        public int getSectorSize() throws IOException
        {
            return underlying.getBlockSize();
        }

        @Override
        public void close() throws IOException
        {

        }

        @Override
        public boolean isClosed()
        {
            return false;
        }

        @Override
        public boolean isReadOnly()
        {
            return false;
        }
    }

    public FatFileSystemImpl(FatFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        this.fatDevice = new BlockDeviceAdapter(blockDevice);
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        SuperFloppyFormatter
                .get(fatDevice)
                .setFatType(FatType.FAT12)
                .setOemName("fluxengn")
                .setVolumeLabel(volumeName)
                .format();
    }
}
