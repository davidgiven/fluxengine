package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.FileSystemImpl.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.FileType.IS_FILE;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.cowlark.fluxengine.vfs.FileSystemImpl.Dirent;
import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class FatFileSystemImplTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;
    private InMemoryBlockDevice blockDevice;
    private FatFileSystemImpl impl;

    @Before
    public void setup()
    {
        configProto =
                new ConfigBuilder().loadConfigFile("ibm").withImageWriter("/tmp/out.img").build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new FatFileSystemImpl(configProto.getFilesystem().getFatfs(), blockDevice);
    }

    @Test
    public void createFilesystem() throws IOException
    {
        impl.create(true, "LABEL");
        blockDevice.commit();
        assertThat(image.get(0, 0, 1).data.reader().seek(3).readString(8)).isEqualTo("MSDOS5.0");
        assertThat(image.get(0, 0, 2).data
                .reader()
                .read(3)
                .toByteArray()).isEqualTo(new byte[]{(byte) 0xf8, (byte) 0xff, (byte) 0xff});
    }

    @Test
    public void getFile() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        Bytes bytes = impl.getFile(Path.of("/data"));
        assertThat(bytes.reader().readString(13)).isEqualTo("Hello, world!");
    }

    @Test
    public void getFile_missing() throws IOException
    {
        impl.create(true, "LABEL");
        assertThrows(NoSuchFileException.class, () -> impl.getFile(Path.of("/data")));
    }

    @Test
    public void putFile() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        blockDevice.commit();
        assertThat(image.get(1, 0, 7).data.reader().readString(13)).isEqualTo("Hello, world!");
    }

    @Test
    public void putFile_replaces() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("This is the wrong data."));
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        blockDevice.commit();
        assertThat(image.get(1, 0, 7).data.reader().readString(13)).isEqualTo("Hello, world!");
    }

    @Test
    public void putFile_onDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/data"));
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        blockDevice.commit();
        assertThat(image.get(1, 0, 8).data.reader().readString(13)).isEqualTo("Hello, world!");
    }

    @Test
    public void createDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir"));
        Dirent de = impl.getDirent(Path.of("/dir"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void createDirectory_fileExists() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.createDirectory(Path.of("/data")));
    }

    @Test
    public void createDirectory_nested() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir1"));
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        assertThat(impl.list(Path.of("/dir1"))).hasSize(0);

        impl.createDirectory(Path.of("/dir1", "dir2"));
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        assertThat(impl.list(Path.of("/dir1"))).hasSize(1);

        Dirent de = impl.getDirent(Path.of("/dir1/dir2"));
        assertThat(de.filename()).isEqualTo("dir2");
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void createDirectory_middleMissing() throws IOException
    {
        impl.create(true, "LABEL");
        assertThrows(
                NoSuchFileException.class,
                () -> impl.createDirectory(Path.of("/dir1", "dir2", "dir3", "dir4")));
    }

    @Test
    public void delete_file() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        impl.deleteFile(Path.of("/data"));
        assertThat(impl.list(Path.of("/"))).isEmpty();
    }

    @Test
    public void delete_dir() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir"));
        impl.deleteFile(Path.of("/dir"));
        assertThat(impl.list(Path.of("/"))).isEmpty();
    }

    @Test
    public void delete_missing() throws IOException
    {
        impl.create(true, "LABEL");
        assertThrows(NoSuchFileException.class, () -> impl.deleteFile(Path.of("/dir")));
    }

    @Test
    public void delete_middle() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir1"));
        impl.createDirectory(Path.of("/dir1", "dir2"));
        assertThrows(DirectoryNotEmptyException.class, () -> impl.deleteFile(Path.of("/dir1")));
    }

    @Test
    public void listFiles() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        ImmutableMap<String, Dirent> files = impl.list(Path.of("/"));
        assertThat(files).hasSize(1);
        assertThat(files.get("data")).isEqualTo(Dirent
                .builder()
                .setPath(Path.of("/data"))
                .setFilename("data")
                .setLength(13)
                .setFileType(IS_FILE)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "data")
                        .put(Attributes.LENGTH, "13")
                        .put(Attributes.FILE_TYPE, "file")
                        .build())
                .build());
    }

    @Test
    public void getDirent() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));
        Dirent de = impl.getDirent(Path.of("/data"));
        assertThat(de).isEqualTo(Dirent
                .builder()
                .setPath(Path.of("/data"))
                .setFilename("data")
                .setLength(13)
                .setFileType(IS_FILE)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "data")
                        .put(Attributes.LENGTH, "13")
                        .put(Attributes.FILE_TYPE, "file")
                        .build())
                .build());
    }

    @Test
    public void flushActuallyFlushes() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir1"));
        impl.createDirectory(Path.of("/dir1/dir2"));
        impl.createDirectory(Path.of("/dir1/dir2/dir3"));
        impl.putFile(Path.of("/dir1/dir2/dir3/data"), new Bytes("Hello, world!"));
        impl.flushChanges();

        FatFileSystemImpl impl2 =
                new FatFileSystemImpl(configProto.getFilesystem().getFatfs(), blockDevice);
        Dirent de = impl2.getDirent(Path.of("/dir1/dir2/dir3/data"));
        assertThat(de.filename()).isEqualTo("data");
    }

    /* Do not use --- for debugging the test only */
    private void writeImage()
    {
        ImageWriter.create(configProto).writeImage(image);
    }
}
