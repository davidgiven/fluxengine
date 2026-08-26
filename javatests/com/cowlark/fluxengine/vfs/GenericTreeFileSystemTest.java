package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.FileSystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.FileSystem.FileType.IS_FILE;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.FileSystem.Dirent;
import com.google.common.collect.ImmutableMap;
import org.junit.Test;
import java.io.IOException;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.nio.file.Path;

public abstract class GenericTreeFileSystemTest
{
    protected BlockDevice blockDevice;
    protected FileSystem impl;

    public abstract void createTestFilesystem();

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
    public void putGetFile() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));

        Bytes output = impl.getFile(Path.of("/data"));
        assertThat(output).isEqualTo(new Bytes("Hello, world!"));
    }

    @Test
    public void putFile_replaces() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/data"), new Bytes("This is the wrong data."));
        impl.putFile(Path.of("/data"), new Bytes("Hello, world!"));

        Bytes output = impl.getFile(Path.of("/data"));
        assertThat(output).isEqualTo(new Bytes("Hello, world!"));
    }

    @Test
    public void putFile_onDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/data"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.putFile(Path.of("/data"), new Bytes("Hello, world!")));
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

        createTestFilesystem();
        Dirent de = impl.getDirent(Path.of("/dir1/dir2/dir3/data"));
        assertThat(de.filename()).isEqualTo("data");
    }

    @Test
    public void discardChanges() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir"));
        impl.flushChanges();

        /* check the directory exists after flush */
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        assertThat(impl.getDirent(Path.of("/dir")).fileType()).isEqualTo(IS_DIR);

        impl.deleteFile(Path.of("/dir"));
        assertThat(impl.list(Path.of("/"))).isEmpty();

        impl.discardChanges();

        /* check that the directory still exists after discard */
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        Dirent de = impl.getDirent(Path.of("/dir"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void moveFileInRoot() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/old.txt"), new Bytes("content old"));
        impl.moveFile(Path.of("/old.txt"), Path.of("/new.txt"));
        assertThrows(NoSuchFileException.class, () -> impl.getFile(Path.of("/old.txt")));

        assertThat(impl.getFile(Path.of("/new.txt")).reader().readString(11)).isEqualTo(
                "content old");
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        assertThat(impl.getDirent(Path.of("/new.txt")).fileType()).isEqualTo(IS_FILE);
    }

    @Test
    public void moveDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir1"));
        impl.putFile(Path.of("/dir1/file.txt"), new Bytes("inside"));
        impl.moveFile(Path.of("/dir1"), Path.of("/dir2"));
        assertThrows(NoSuchFileException.class, () -> impl.getDirent(Path.of("/dir1")));

        Dirent de = impl.getDirent(Path.of("/dir2"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
        assertThat(impl.getFile(Path.of("/dir2/file.txt")).reader().readString(6)).isEqualTo(
                "inside");
        assertThat(impl.list(Path.of("/"))).hasSize(1);
    }

    @Test
    public void moveFileIntoDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/file.txt"), new Bytes("hello"));
        impl.createDirectory(Path.of("/dir"));
        impl.moveFile(Path.of("/file.txt"), Path.of("/dir/file.txt"));
        assertThrows(NoSuchFileException.class, () -> impl.getFile(Path.of("/file.txt")));

        assertThat(impl
                .getFile(Path.of("/dir/file.txt"))
                .reader()
                .readString(5)).isEqualTo("hello");
        assertThat(impl.list(Path.of("/"))).hasSize(1);
        assertThat(impl.list(Path.of("/dir"))).hasSize(1);
    }

    @Test
    public void moveDirectoryIntoItself() throws IOException
    {
        impl.create(true, "LABEL");
        impl.createDirectory(Path.of("/dir"));
        impl.createDirectory(Path.of("/dir/sub"));

        /* Moving /dir into its own subtree should fail (e.g., /dir -> /dir/sub/moved) */
        assertThrows(
                InvalidPathException.class,
                () -> impl.moveFile(Path.of("/dir"), Path.of("/dir/sub/moved")));

        /* Also moving directly onto itself should fail */
        assertThrows(
                InvalidPathException.class,
                () -> impl.moveFile(Path.of("/dir"), Path.of("/dir")));

        /* Original still exists and is intact */
        assertThat(impl.getDirent(Path.of("/dir")).fileType()).isEqualTo(IS_DIR);
        assertThat(impl.getDirent(Path.of("/dir/sub")).fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void moveFileOnTopOfAnotherFile() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/a.txt"), new Bytes("AAA"));
        impl.putFile(Path.of("/b.txt"), new Bytes("BBB"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(Path.of("/a.txt"), Path.of("/b.txt")));

        /* Both files should still exist with original contents */
        assertThat(impl.getFile(Path.of("/a.txt")).reader().readString(3)).isEqualTo("AAA");
        assertThat(impl.getFile(Path.of("/b.txt")).reader().readString(3)).isEqualTo("BBB");
    }

    @Test
    public void moveFileOnTopOfDirectory() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("/file.txt"), new Bytes("data"));
        impl.createDirectory(Path.of("/dir"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(Path.of("/file.txt"), Path.of("/dir")));

        /* Also moving a directory onto a file should fail */
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(Path.of("/dir"), Path.of("/file.txt")));
        assertThat(impl.getFile(Path.of("/file.txt")).reader().readString(4)).isEqualTo("data");
        assertThat(impl.getDirent(Path.of("/dir")).fileType()).isEqualTo(IS_DIR);
    }
}
