package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.dfs.Dfs;
import org.rob_smallshire.oaknut.dfs.DfsFormat;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import com.google.common.collect.ImmutableMap;
import java.io.IOException;
import java.nio.file.AccessDeniedException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.List;

@RunWith(JUnit4.class)
public class AcornDfsFilesystemTest extends GenericFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto = new ConfigBuilder()
                .loadConfigFile("acorndfs")
                .withImageWriter("/tmp/out.img")
                .build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new AcornDfsFilesystem(configProto.getFilesystem().getAcorndfs(), blockDevice);
    }
    /* DFS's directory byte is part of a flat filename; only the default prefix is hidden. */
    @Test
    public void namesAreFlatAndDefaultDirectoryIsStripped() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(VfsPath.of("/Hello"), new Bytes("default"));
        impl.putFile(VfsPath.of("/a.Hello"), new Bytes("letter"));
        assertThat(impl.list(VfsPath.root()).keySet()).containsExactly("Hello", "a.Hello");
        assertThat(impl.getFile(VfsPath.of("/hello"))).isEqualTo(new Bytes("default"));
        assertThat(impl.getFile(VfsPath.of("/A.HELLO"))).isEqualTo(new Bytes("letter"));
        assertThat(impl.getDirent(VfsPath.of("/$.hello")).path()).isEqualTo(VfsPath.of("/Hello"));
        assertThat(impl.getDirent(VfsPath.of("/A.HELLO")).path()).isEqualTo(VfsPath.of("/a.Hello"));
        assertThat(impl.getDirent(VfsPath.of("/A.HELLO")).filename()).isEqualTo("a.Hello");
        assertThrows(NoSuchFileException.class, () -> impl.list(VfsPath.of("/A")));
        assertThrows(InvalidPathException.class, () -> impl.getFile(VfsPath.of("/A/Hello")));
        assertThrows(InvalidPathException.class, () -> impl.putFile(VfsPath.of("/A/Hello"), new Bytes()));
        assertThrows(InvalidPathException.class, () -> impl.moveFile(VfsPath.of("/Hello"), VfsPath.of("/A/Hello")));
        assertThrows(InvalidPathException.class, () -> impl.putFile(VfsPath.of("/file.txt"), new Bytes()));
        impl.moveFile(VfsPath.of("/Hello"), VfsPath.of("/B.New"));
        assertThat(impl.getFile(VfsPath.of("/B.New"))).isEqualTo(new Bytes("default"));
        impl.moveFile(VfsPath.of("/B.New"), VfsPath.of("/$.Root"));
        assertThat(impl.list(VfsPath.root()).keySet()).containsExactly("Root", "a.Hello");
    }

    /* Addresses are raw DFS metadata even when their bit patterns resemble RISC OS types. */
    @Test
    public void addressesAndLockMetadataPersistWithoutFileTypes() throws IOException
    {
        VfsPath path = VfsPath.of("/DATA");
        impl.create(true, "LABEL");
        impl.putFile(path, new Bytes("payload"));
        impl.putFileMetadata(path, ImmutableMap.of(
                AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS, "0xffff1900",
                AcornDfsFilesystem.ACORNDFS_EXEC_ADDRESS, "0x28023",
                FileAttributes.MODE.name(), "L"));
        ImmutableMap<String, String> attributes = impl.getDirent(path).attributes();
        assertThat(attributes.get(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS)).isEqualTo("0xffff1900");
        assertThat(attributes.get(AcornDfsFilesystem.ACORNDFS_EXEC_ADDRESS)).isEqualTo("0x28023");
        assertThat(attributes.get(FileAttributes.MODE.name())).isEqualTo("LR/");
        assertThat(attributes).doesNotContainKey("acorndfs.filetype");
        assertThat(attributes).doesNotContainKey(FileAttributes.CTIME.name());
        assertThat(impl.list(VfsPath.root()).get("DATA").attributes()).isEqualTo(attributes);
        assertThrows(AccessDeniedException.class, () -> impl.deleteFile(path));
        assertThrows(AccessDeniedException.class, () -> impl.putFile(path, new Bytes("replacement")));
        assertThrows(IllegalArgumentException.class, () -> impl.putFileMetadata(path,
                ImmutableMap.of("acorndfs.filetype", "0xabc")));
        impl.putFileMetadata(path, ImmutableMap.of(FileAttributes.MODE.name(), "WR"));
        impl.putFile(path, new Bytes("replacement"));
        impl.putFileMetadata(path, ImmutableMap.of(AcornDfsFilesystem.ACORNDFS_EXEC_ADDRESS, "0xffff8023"));
        impl.flushChanges();
        createTestFilesystem();
        assertThat(impl.getFile(path)).isEqualTo(new Bytes("replacement"));
        assertThat(impl.getDirent(path).attributes().get(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS))
                .isEqualTo("0xffff1900");
        assertThat(impl.getDirent(path).attributes().get(AcornDfsFilesystem.ACORNDFS_EXEC_ADDRESS))
                .isEqualTo("0xffff8023");
        assertThat(impl.getDirent(path).attributes()).doesNotContainKey("acorndfs.filetype");
        assertThat(impl.getDirent(path).attributes()).doesNotContainKey(FileAttributes.CTIME.name());
    }

    @Test
    public void formattingAndVolumeMetadataAtBothCapacities() throws IOException
    {
        for (String capacity : List.of("100", "200"))
        {
            ConfigProto selected = new ConfigBuilder().loadConfigFile("acorndfs").applyOption(capacity, "").build();
            BlockDevice device = new InMemoryBlockDevice(new DiskLayout(selected), new Image());
            try (AcornDfsFilesystem fs = new AcornDfsFilesystem(selected.getFilesystem().getAcorndfs(), device))
            {
                fs.create(true, "LABEL");
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("LABEL");
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.TOTAL_BLOCKS.name()))
                        .isEqualTo(capacity.equals("100") ? "400" : "800");
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.USED_BLOCKS.name())).isEqualTo("2");
                fs.putFile(VfsPath.of("/DATA"), new Bytes(257));
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.USED_BLOCKS.name())).isEqualTo("4");
                fs.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "NEW LABEL"));
                fs.flushChanges();
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("NEW LABEL");
                fs.create(false, "FULL");
                assertThat(fs.list(VfsPath.root())).isEmpty();
                assertThat(fs.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("FULL");
                assertThat(fs.isReadOnly()).isFalse();
                fs.check();
            }
        }
    }

    @Test
    public void discardRestoresCommittedDataAndAddresses() throws IOException
    {
        VfsPath path = VfsPath.of("/A.DATA");
        impl.create(true, "LABEL");
        impl.putFile(path, new Bytes("committed"));
        impl.putFileMetadata(path, ImmutableMap.of(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS, "0x1900"));
        impl.flushChanges();
        assertThat(impl.needsFlushing()).isFalse();
        impl.putFile(path, new Bytes("pending"));
        impl.putFileMetadata(path, ImmutableMap.of(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS, "0x2000"));
        impl.putFilesystemMetadata(ImmutableMap.of(FilesystemAttributes.VOLUME_NAME.name(), "PENDING"));
        assertThat(impl.needsFlushing()).isTrue();
        impl.discardChanges();
        assertThat(impl.needsFlushing()).isFalse();
        assertThat(impl.getFile(path)).isEqualTo(new Bytes("committed"));
        assertThat(impl.getDirent(path).attributes().get(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS)).isEqualTo("0x1900");
        assertThat(impl.getFilesystemMetadata().get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("LABEL");
        impl.deleteFile(path);
        impl.discardChanges();
        assertThat(impl.getFile(path)).isEqualTo(new Bytes("committed"));
    }

    @Test
    public void readsExistingOaknutImagesAndMalformedNamesFail() throws IOException
    {
        try (Dfs disc = Dfs.create(DfsFormat.ACORN_DFS_80T_SINGLE_SIDED))
        {
            disc.writeFile("$.PLAIN", new byte[]{1}, new AcornMeta(0xffff1900L, 0x28023, Access.L));
            disc.writeFile("B.LETTER", new byte[]{2});
            blockDevice.putBytes(0, new Bytes(disc.toByteArray()));
        }
        assertThat(impl.list(VfsPath.root()).keySet()).containsExactly("PLAIN", "B.LETTER");
        assertThat(impl.getFile(VfsPath.of("/PLAIN"))).isEqualTo(new Bytes(new byte[]{1}));
        assertThat(impl.getDirent(VfsPath.of("/PLAIN")).attributes().get(AcornDfsFilesystem.ACORNDFS_LOAD_ADDRESS))
                .isEqualTo("0xffff1900");
        for (String invalid : List.of("/TOOLONGNAME", "/A.BAD.NAME", "/BAD:NAME"))
            assertThrows(InvalidPathException.class, () -> impl.putFile(VfsPath.of(invalid), new Bytes()));
    }

    @Test
    public void quickFormatDoesNotReadDiskAndOperationsRemainLazy() throws IOException
    {
        CountingBlockDevice device = new CountingBlockDevice(diskLayout, new Image());
        /* An old Watford catalogue must not be mistaken for the newly formatted volume. */
        byte[] stale = new byte[1024];
        java.util.Arrays.fill(stale, (byte) 0xaa);
        device.putBytes(0, new Bytes(stale));
        try (AcornDfsFilesystem fs = new AcornDfsFilesystem(configProto.getFilesystem().getAcorndfs(), device))
        {
            assertThat(device.reads).isEqualTo(0);
            fs.create(true, "LABEL");
            assertThat(device.reads).isEqualTo(0);
            fs.list(VfsPath.root());
            fs.putFile(VfsPath.of("/DATA"), new Bytes("payload"));
            assertThat(device.reads).isLessThan(device.getBlockCount());
            assertThat(fs.getFile(VfsPath.of("/DATA"))).isEqualTo(new Bytes("payload"));
            device.failReads = true;
            assertThat(assertThrows(IOException.class, () -> fs.getFile(VfsPath.of("/DATA"))).getMessage())
                    .isEqualTo("read failed");
        }
    }

    private static final class CountingBlockDevice extends InMemoryBlockDevice
    {
        int reads;
        boolean failReads;

        CountingBlockDevice(DiskLayout layout, Image image)
        {
            super(layout, image);
        }

        @Override
        public Bytes getBlock(int number) throws IOException
        {
            if (failReads)
                throw new IOException("read failed");
            reads++;
            return super.getBlock(number);
        }
    }
}
