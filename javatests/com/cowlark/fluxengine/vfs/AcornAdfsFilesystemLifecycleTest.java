package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.adfs.Adfs;
import org.rob_smallshire.oaknut.adfs.AdfsFormat;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.util.Arrays;
import java.util.HashSet;
import java.util.Set;

@RunWith(JUnit4.class)
public class AcornAdfsFilesystemLifecycleTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto config;
    private RecordingBlockDevice device;
    private AcornAdfsFilesystem filesystem;

    @Before
    public void setup() throws IOException
    {
        /* Generate a valid floppy in memory; no stored disk image fixtures are needed. */
        config = new ConfigBuilder().loadConfigFile("acornadfs").build();
        try (Adfs image = Adfs.create(AdfsFormat.F))
        {
            device = new RecordingBlockDevice(new DiskLayout(config), image.toByteArray());
        }
        filesystem = new AcornAdfsFilesystem(config.getFilesystem().getAcornadfs(), device);
    }

    @Test
    public void mountsLazilyAndReusesTheHandle() throws IOException
    {
        assertThat(device.reads).isEqualTo(0);
        assertThat(filesystem.getCapabilities()).contains(Filesystem.Capability.OP_GETFILE);
        filesystem.getFilesystemMetadata();
        int mountedReads = device.reads;
        assertThat(mountedReads).isGreaterThan(0);
        assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
        filesystem.list(VfsPath.of("/"));
        assertThat(device.reads).isGreaterThan(mountedReads);
        assertThat(device.reads).isLessThan(mountedReads * 2);
        assertThat(device.writes).isEqualTo(0);
        filesystem.close();
    }

    @Test
    public void creationDoesNotTryToMount() throws IOException
    {
        Arrays.fill(device.image, (byte) 0);
        filesystem.create(true, "LABEL");
        assertThat(device.reads).isEqualTo(0);
        filesystem.close();
    }

    @Test
    public void invalidImageMountCanBeRetried() throws IOException
    {
        byte[] valid = device.image.clone();
        Arrays.fill(device.image, (byte) 0);
        assertThrows(IOException.class, filesystem::getFilesystemMetadata);
        int failedReads = device.reads;
        device.image = valid;
        filesystem.getFilesystemMetadata();
        assertThat(device.reads).isGreaterThan(failedReads);
        assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
        filesystem.close();
    }

    @Test
    public void readFailureCanBeRetried() throws IOException
    {
        device.failRead = true;
        IOException error = assertThrows(IOException.class, filesystem::getFilesystemMetadata);
        assertThat(error).hasMessageThat().isEqualTo("read failed");
        device.failRead = false;
        filesystem.getFilesystemMetadata();
        assertThat(device.reads).isGreaterThan(1);
        assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
        filesystem.close();
    }

    @Test
    public void flushCommitsAndInvalidatesTheMount() throws IOException
    {
        filesystem.getFilesystemMetadata();
        int mountedReads = device.reads;
        device.dirty = true;
        assertThat(filesystem.needsFlushing()).isTrue();
        filesystem.flushChanges();
        assertThat(device.commits).isEqualTo(1);
        assertThat(filesystem.needsFlushing()).isFalse();

        /* Replacing the backing bytes must be observed after dismounting. */
        Arrays.fill(device.image, (byte) 0);
        assertThrows(IOException.class, filesystem::getFilesystemMetadata);
        assertThat(device.reads).isGreaterThan(mountedReads);
        assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
        assertThat(device.writes).isEqualTo(0);
        filesystem.close();
    }

    @Test
    public void discardRevertsAndInvalidatesTheMount() throws IOException
    {
        filesystem.getFilesystemMetadata();
        int mountedReads = device.reads;
        device.dirty = true;
        filesystem.discardChanges();
        assertThat(device.reverts).isEqualTo(1);
        assertThat(filesystem.needsFlushing()).isFalse();
        filesystem.getFilesystemMetadata();
        assertThat(device.reads).isEqualTo(mountedReads * 2);
        assertThat(device.writes).isEqualTo(0);
        filesystem.close();
    }

    @Test
    public void unmountedLifecycleDoesNotReadTheDisk() throws IOException
    {
        filesystem.flushChanges();
        filesystem.discardChanges();
        filesystem.close();
        filesystem.close();
        assertThat(device.reads).isEqualTo(0);
        assertThat(device.writes).isEqualTo(0);
        assertThat(device.commits).isEqualTo(3);
        assertThat(device.reverts).isEqualTo(1);
    }

    @Test
    public void closeCommitsAndDismounts() throws IOException
    {
        filesystem.getFilesystemMetadata();
        int mountedReads = device.reads;
        filesystem.close();
        assertThat(device.commits).isEqualTo(1);
        filesystem.getFilesystemMetadata();
        assertThat(device.reads).isEqualTo(mountedReads * 2);
        filesystem.close();
    }

    @Test
    public void configurationSelectsTheAdapter() throws IOException
    {
        assertThat(config
                .getFilesystem()
                .getType()).isEqualTo(FilesystemProto.FilesystemType.ACORNADFS);
        FilesystemProto proto = FilesystemProto
                .newBuilder()
                .setType(FilesystemProto.FilesystemType.ACORNADFS)
                .setAcornadfs(AcornAdfsProto.getDefaultInstance())
                .build();
        assertThat(FilesystemProto.parseFrom(proto.toByteArray())).isEqualTo(proto);

        /* Exercise the actual factory switch without opening an image reader or physical drive. */
        FilesystemOperation operation = new FilesystemOperation()
        {
            @Override
            protected BlockDevice createBlockDevice()
            {
                return device;
            }

            @Override
            public void run(Filesystem selected) throws IOException
            {
                assertThat(selected).isInstanceOf(AcornAdfsFilesystem.class);
                assertThat(selected
                        .getFilesystemMetadata()
                        .get(FilesystemAttributes.VOLUME_NAME.name())).isEmpty();
            }
        };
        operation.setConfig(config);
        operation.run();
        assertThat(device.commits).isEqualTo(1);
        assertThat(device.reads).isGreaterThan(0);
        assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
    }

    @Test
    public void storageWritesThroughAndBlockDeviceCanRevert() throws IOException
    {
        byte[] original = device.image.clone();
        try (Adfs image = Adfs.fromStorage(
                new AcornAdfsFilesystem.OaknutBlockDeviceStorage(device),
                false))
        {
            image.path("$.New").writeBytes(new byte[]{1, 2, 3});
            assertThat(device.dirty).isTrue();
            assertThat(device.writes).isGreaterThan(0);
            assertThat(device.readBlocks.size()).isLessThan(device.getBlockCount());
            assertThat(image.path("$.New").readBytes()).isEqualTo(new byte[]{1, 2, 3});
        }
        assertThat(device.commits).isEqualTo(0);
        device.revert();
        assertThat(device.image).isEqualTo(original);
        try (Adfs image = Adfs.fromStorage(
                new AcornAdfsFilesystem.OaknutBlockDeviceStorage(device),
                false))
        {
            assertThat(image.path("$.New").exists()).isFalse();
        }
    }

    @Test
    public void adapterReadsAndWritesAcrossBlockBoundaries() throws IOException
    {
        AcornAdfsFilesystem.OaknutBlockDeviceStorage storage =
                new AcornAdfsFilesystem.OaknutBlockDeviceStorage(device);
        int offset = device.getBlockSize() - 2;
        byte[] original = device.image.clone();
        byte[] changed = {9, 8, 7, 6};
        storage.write(offset, changed, 0, changed.length);
        byte[] read = new byte[4];
        storage.read(offset, read, 0, read.length);
        assertThat(read).isEqualTo(changed);
        assertThat(device.reads).isEqualTo(4);
        assertThat(device.writes).isEqualTo(2);
        assertThat(device.image[offset - 1]).isEqualTo(original[offset - 1]);
        assertThat(device.image[offset + 4]).isEqualTo(original[offset + 4]);
    }

    @Test
    public void fullBlockWriteDoesNotReadAndFailedWritesDoNotRetainData() throws IOException
    {
        AcornAdfsFilesystem.OaknutBlockDeviceStorage storage =
                new AcornAdfsFilesystem.OaknutBlockDeviceStorage(device);
        byte[] block = new byte[device.getBlockSize()];
        storage.write(0, block, 0, block.length);
        assertThat(device.reads).isEqualTo(0);
        assertThat(device.writes).isEqualTo(1);

        device.failWrite = true;
        assertThrows(IOException.class, () -> storage.write(0, new byte[]{9}, 0, 1));
        device.image[0] = 7;
        byte[] read = new byte[1];
        storage.read(0, read, 0, 1);
        assertThat(read[0]).isEqualTo((byte) 7);
        assertThat(device.reads).isEqualTo(2);
    }

    @Test
    public void adapterAlwaysObservesCurrentBlockDeviceData() throws IOException
    {
        AcornAdfsFilesystem.OaknutBlockDeviceStorage storage =
                new AcornAdfsFilesystem.OaknutBlockDeviceStorage(device);
        byte[] first = new byte[1];
        storage.read(0, first, 0, 1);
        device.image[0] = (byte) (first[0] ^ 255);
        byte[] second = new byte[1];
        storage.read(0, second, 0, 1);
        assertThat(second[0]).isEqualTo(device.image[0]);
        assertThat(device.reads).isEqualTo(2);

        /* Partial writes must preserve changes made through the underlying device. */
        storage.write(1, new byte[]{42}, 0, 1);
        assertThat(device.image[0]).isEqualTo(second[0]);
        assertThat(device.image[1]).isEqualTo((byte) 42);
    }

    /* Track distinct blocks as well as calls: BlockDevice owns repeated-access caching. */
    private static class RecordingBlockDevice extends BlockDevice
    {
        final Set<Integer> readBlocks = new HashSet<>();
        byte[] image;
        byte[] committedImage;
        int reads;
        int writes;
        int commits;
        int reverts;
        boolean dirty;
        boolean failRead;
        boolean failWrite;

        RecordingBlockDevice(DiskLayout layout, byte[] image)
        {
            super(layout);
            this.image = image;
            committedImage = image.clone();
        }

        @Override
        public int getBlockCount()
        {
            return image.length / getBlockSize();
        }

        @Override
        public Bytes getBlocks(int blockNumber, int count)
        {
            throw new AssertionError("Mount must not read the complete disk with getBlocks");
        }

        @Override
        public Bytes getBlock(int blockNumber) throws IOException
        {
            reads++;
            readBlocks.add(blockNumber);
            if (failRead)
                throw new IOException("read failed");
            int start = blockNumber * getBlockSize();
            return new Bytes(Arrays.copyOfRange(image, start, start + getBlockSize()));
        }

        @Override
        public void putBlock(int blockNumber, Bytes block) throws FileSystemException
        {
            writes++;
            if (failWrite)
                throw new FileSystemException("write failed");
            dirty = true;
            System.arraycopy(
                    block.toByteArray(),
                    0,
                    image,
                    blockNumber * getBlockSize(),
                    getBlockSize());
        }

        @Override
        public void commit()
        {
            commits++;
            committedImage = image.clone();
            dirty = false;
        }

        @Override
        public void revert()
        {
            reverts++;
            image = committedImage.clone();
            dirty = false;
        }

        @Override
        public boolean needsCommit()
        {
            return dirty;
        }
    }
}
