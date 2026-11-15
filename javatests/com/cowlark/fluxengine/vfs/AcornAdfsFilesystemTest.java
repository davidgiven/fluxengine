package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.rob_smallshire.oaknut.adfs.Adfs;
import org.rob_smallshire.oaknut.adfs.AdfsFormat;
import java.io.IOException;
import java.nio.file.AccessDeniedException;
import java.nio.file.InvalidPathException;
import java.time.Instant;
import java.util.List;

@RunWith(JUnit4.class)
public class AcornAdfsFilesystemTest extends GenericTreeFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();
    private ConfigProto config;
    private DiskLayout layout;
    private Image image;

    @Before
    public void setup()
    {
        config = new ConfigBuilder().loadConfigFile("acornadfs").build();
        layout = new DiskLayout(config);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(layout, image);
        impl = new AcornAdfsFilesystem(config.getFilesystem().getAcornadfs(), blockDevice);
    }

    @Test
    public void filesystemMetadataTracksUsageAndPersistsTitle() throws IOException
    {
        impl.create(true, "LABEL");
        ImmutableMap<String, String> initial = impl.getFilesystemMetadata();
        assertThat(initial.get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("LABEL");
        assertThat(initial.get(FilesystemAttributes.BLOCK_SIZE.name())).isEqualTo("256");
        assertThat(initial.get(FilesystemAttributes.TOTAL_BLOCKS.name())).isEqualTo("6400");
        impl.putFile(VfsPath.of("/data"), new Bytes(5000));
        assertThat(Integer.parseInt(impl
                .getFilesystemMetadata()
                .get(FilesystemAttributes.USED_BLOCKS.name()))).isGreaterThan(Integer.parseInt(
                initial.get(FilesystemAttributes.USED_BLOCKS.name())));
        impl.putFilesystemMetadata(ImmutableMap.of(
                FilesystemAttributes.VOLUME_NAME.name(),
                "NEW TITLE"));
        impl.flushChanges();
        createTestFilesystem();
        assertThat(impl
                .getFilesystemMetadata()
                .get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("NEW TITLE");
        assertThrows(
                IllegalArgumentException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of("UNKNOWN", "x")));
        assertThrows(
                InvalidPathException.class,
                () -> impl.putFilesystemMetadata(ImmutableMap.of(
                        FilesystemAttributes.VOLUME_NAME.name(),
                        "TITLE THAT IS TOO LONG")));
    }

    @Test
    public void pathsUseLiteralComponents() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dirone"));
        impl.createDirectory(VfsPath.of("/dirone/@"));
        VfsPath path = VfsPath.of("/dirone/@/file");
        impl.putFile(path, new Bytes("payload"));
        assertThat(impl.getFile(path)).isEqualTo(new Bytes("payload"));
        assertThat(impl.list(VfsPath.of("/dirone/@")).get("file").path()).isEqualTo(path);
        impl.flushChanges();
        createTestFilesystem();
        assertThat(impl.getDirent(path).path()).isEqualTo(path);
        impl.moveFile(path, VfsPath.of("/dirone/@/other"));
        assertThat(impl.getFile(VfsPath.of("/dirone/@/other"))).isEqualTo(new Bytes("payload"
        ));
    }

    /* Reject invalid names before any file, directory, or rename changes are made. */
    @Test
    public void dottedNamesAreRejected() throws IOException
    {
        impl.create(true, "LABEL");
        VfsPath source = VfsPath.of("/source");
        impl.putFile(source, new Bytes("payload"));
        for (String name : List.of("file.txt", "dir.one", ".hidden", "trailing."))
        {
            VfsPath destination = VfsPath.root().resolve(name);
            assertThrows(InvalidPathException.class,
                    () -> impl.putFile(destination, new Bytes("data")));
            assertThrows(InvalidPathException.class,
                    () -> impl.createDirectory(destination));
            assertThrows(InvalidPathException.class,
                    () -> impl.moveFile(source, destination));
        }
        assertThat(impl.getFile(source)).isEqualTo(new Bytes("payload"));
        assertThat(impl.list(VfsPath.root()).keySet()).containsExactly("source");
    }

    @Test
    public void modeAndTimestampMetadataArePreserved() throws IOException
    {
        impl.create(true, "LABEL");
        VfsPath path = VfsPath.of("/typed");
        impl.putFile(path, new Bytes("data"));
        String time = Instant.parse("2024-02-03T12:34:56.780Z").toString();
        impl.putFileMetadata(
                path,
                ImmutableMap.of(
                        FileAttributes.MODE.name(),
                        "LWR/R",
                        FileAttributes.CTIME.name(),
                        time));
        assertThat(impl.getDirent(path).mode()).isEqualTo("LWR/R");
        assertThat(impl.getDirent(path).attributes().get(FileAttributes.CTIME.name())).isEqualTo(
                time);
        assertThrows(AccessDeniedException.class, () -> impl.deleteFile(path));
        assertThrows(
                AccessDeniedException.class,
                () -> impl.putFile(path, new Bytes("replacement")));
        impl.putFileMetadata(path, ImmutableMap.of(FileAttributes.MODE.name(), "WR/R"));
        impl.putFile(path, new Bytes("replacement"));
        impl.flushChanges();
        createTestFilesystem();
        assertThat(impl.getFile(path)).isEqualTo(new Bytes("replacement"));
        assertThat(impl.getDirent(path).attributes().get(FileAttributes.CTIME.name())).isEqualTo(
                time);
    }

    @Test
    public void quickAndFullFormattingWorkAtEveryConfiguredCapacity() throws IOException
    {
        for (String capacity : List.of("160", "320", "640", "800", "1600"))
        {
            ConfigProto selected = new ConfigBuilder()
                    .loadConfigFile("acornadfs")
                    .applyOption(capacity, "")
                    .build();
            BlockDevice device = new InMemoryBlockDevice(new DiskLayout(selected), new Image());
            try (AcornAdfsFilesystem fs = new AcornAdfsFilesystem(
                    selected
                            .getFilesystem()
                            .getAcornadfs(), device))
            {
                fs.create(true, "QUICK");
                fs.createDirectory(VfsPath.of("/dir"));
                fs.putFile(VfsPath.of("/dir/file"), new Bytes("data"));
                fs.flushChanges();
                assertThat(fs.getFile(VfsPath.of("/dir/file"))).isEqualTo(new Bytes("data"));
                fs.create(false, "FULL");
                assertThat(fs.list(VfsPath.root())).isEmpty();
                assertThat(fs
                        .getFilesystemMetadata()
                        .get(FilesystemAttributes.VOLUME_NAME.name())).isEqualTo("FULL");
                assertThat(fs.isReadOnly()).isFalse();
                fs.check();
            }
        }
    }

    @Test
    public void existingDAndBigDirectoriesSupportNormalVfsOperations() throws IOException
    {
        for (AdfsFormat format : List.of(AdfsFormat.D, AdfsFormat.E_PLUS, AdfsFormat.F_PLUS))
        {
            ConfigProto selected = new ConfigBuilder()
                    .loadConfigFile("acornadfs")
                    .applyOption(format.size() == 819200 ? "800" : "1600", "")
                    .build();
            BlockDevice device = new InMemoryBlockDevice(new DiskLayout(selected), new Image());
            try (Adfs blank = Adfs.create(format))
            {
                device.putBytes(0, new Bytes(blank.toByteArray()));
            }
            device.commit();
            try (AcornAdfsFilesystem fs = new AcornAdfsFilesystem(
                    selected
                            .getFilesystem()
                            .getAcornadfs(), device))
            {
                fs.createDirectory(VfsPath.of("/dirone"));
                fs.putFile(VfsPath.of("/dirone/file"), new Bytes("data"));
                fs.moveFile(VfsPath.of("/dirone/file"), VfsPath.of("/dirone/other"));
                assertThat(fs.getFile(VfsPath.of("/dirone/other"))).isEqualTo(new Bytes(
                        "data"));
                assertThat(fs.list(VfsPath.of("/dirone")).get("other").path()).isEqualTo(
                        VfsPath.of("/dirone/other"));
                fs.deleteFile(VfsPath.of("/dirone/other"));
                fs.deleteFile(VfsPath.of("/dirone"));
                assertThat(fs.list(VfsPath.root())).isEmpty();
            }
        }
    }
}
