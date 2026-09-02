package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.Filesystem.Capability;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.google.common.collect.ImmutableMap;
import org.junit.Assume;
import org.junit.Test;
import java.io.IOException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.NoSuchFileException;

public abstract class GenericFilesystemTest
{
    protected BlockDevice blockDevice;
    protected Filesystem impl;

    public abstract void createTestFilesystem();

    protected Bytes getTestFileData(String contents)
    {
        return new Bytes(contents);
    }

    protected Bytes getTestFileData()
    {
        return getTestFileData("Test data!");
    }

    private void assume(Capability... caps)
    {
        for (Capability c : caps)
            Assume.assumeTrue(impl.getCapabilities().contains(c));
    }

    @Test
    public void getFile_missing() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_GETFILE);
        impl.create(true, "LABEL");
        assertThrows(NoSuchFileException.class, () -> impl.getFile(VfsPath.of("/data")));
    }

    @Test
    public void putGetFile() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_PUTFILE, Capability.OP_GETFILE);
        impl.create(true, "LABEL");
        Bytes expected = getTestFileData();
        impl.putFile(VfsPath.of("/data"), expected);

        Bytes output = impl.getFile(VfsPath.of("/data"));
        assertThat(output).isEqualTo(expected);
    }

    @Test
    public void putFile_replaces() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_PUTFILE, Capability.OP_GETFILE);
        impl.create(true, "LABEL");
        impl.putFile(VfsPath.of("/data"), getTestFileData("This is the wrong data."));
        Bytes expected = getTestFileData();
        impl.putFile(VfsPath.of("/data"), expected);

        Bytes output = impl.getFile(VfsPath.of("/data"));
        assertThat(output).isEqualTo(expected);
    }

    @Test
    public void delete_file() throws IOException
    {
        assume(
                Capability.OP_CREATE,
                Capability.OP_PUTFILE,
                Capability.OP_DELETE,
                Capability.OP_LIST);
        impl.create(true, "LABEL");
        impl.putFile(VfsPath.of("/data"), getTestFileData());
        impl.deleteFile(VfsPath.of("/data"));
        assertThat(impl.list(VfsPath.of("/"))).isEmpty();
    }

    @Test
    public void delete_missing() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_DELETE);
        impl.create(true, "LABEL");
        assertThrows(NoSuchFileException.class, () -> impl.deleteFile(VfsPath.of("/dir")));
    }

    @Test
    public void listFiles() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_PUTFILE, Capability.OP_LIST);
        impl.create(true, "LABEL");
        Bytes expected = getTestFileData();
        impl.putFile(VfsPath.of("/data"), expected);
        ImmutableMap<String, Dirent> files = impl.list(VfsPath.of("/"));
        assertThat(files).hasSize(1);
        assertThat(files.get("data")).isEqualTo(Dirent
                .builder()
                .setPath(VfsPath.of("/data"))
                .setFilename("data")
                .setLength(expected.size())
                .setFileType(IS_FILE)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "data")
                        .put(Attributes.LENGTH, Integer.toString(expected.size()))
                        .put(Attributes.FILE_TYPE, "file")
                        .build())
                .build());
    }

    @Test
    public void getDirent() throws IOException
    {
        assume(Capability.OP_CREATE, Capability.OP_PUTFILE, Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        Bytes expected = getTestFileData();
        impl.putFile(VfsPath.of("/data"), expected);
        Dirent de = impl.getDirent(VfsPath.of("/data"));
        assertThat(de).isEqualTo(Dirent
                .builder()
                .setPath(VfsPath.of("/data"))
                .setFilename("data")
                .setLength(expected.size())
                .setFileType(IS_FILE)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "data")
                        .put(Attributes.LENGTH, Integer.toString(expected.size()))
                        .put(Attributes.FILE_TYPE, "file")
                        .build())
                .build());
    }

    @Test
    public void flushActuallyFlushes() throws IOException
    {
        assume(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_PUTFILE,
                Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir1"));
        impl.createDirectory(VfsPath.of("/dir1/dir2"));
        impl.createDirectory(VfsPath.of("/dir1/dir2/dir3"));
        impl.putFile(VfsPath.of("/dir1/dir2/dir3/data"), getTestFileData());
        impl.flushChanges();

        createTestFilesystem();
        Dirent de = impl.getDirent(VfsPath.of("/dir1/dir2/dir3/data"));
        assertThat(de.filename()).isEqualTo("data");
    }

    @Test
    public void discardChanges() throws IOException
    {
        assume(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_LIST,
                Capability.OP_GETDIRENT,
                Capability.OP_DELETE);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir"));
        impl.flushChanges();

        /* check the directory exists after flush */
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.getDirent(VfsPath.of("/dir")).fileType()).isEqualTo(IS_DIR);

        impl.deleteFile(VfsPath.of("/dir"));
        assertThat(impl.list(VfsPath.of("/"))).isEmpty();

        impl.discardChanges();

        /* check that the directory still exists after discard */
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        Dirent de = impl.getDirent(VfsPath.of("/dir"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void moveFileInRoot() throws IOException
    {
        assume(
                Capability.OP_CREATE,
                Capability.OP_PUTFILE,
                Capability.OP_MOVE,
                Capability.OP_GETFILE,
                Capability.OP_LIST,
                Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        Bytes data = getTestFileData("content old");
        impl.putFile(VfsPath.of("/old.txt"), data);
        impl.moveFile(VfsPath.of("/old.txt"), VfsPath.of("/new.txt"));
        assertThrows(NoSuchFileException.class, () -> impl.getFile(VfsPath.of("/old.txt")));

        assertThat(impl.getFile(VfsPath.of("/new.txt"))).isEqualTo(data);
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.getDirent(VfsPath.of("/new.txt")).fileType()).isEqualTo(IS_FILE);
    }

    @Test
    public void moveFileOnTopOfAnotherFile() throws IOException
    {
        assume(
                Capability.OP_CREATE,
                Capability.OP_PUTFILE,
                Capability.OP_MOVE,
                Capability.OP_GETFILE);
        impl.create(true, "LABEL");
        Bytes a = getTestFileData("AAA");
        Bytes b = getTestFileData("BBB");
        impl.putFile(VfsPath.of("/a.txt"), a);
        impl.putFile(VfsPath.of("/b.txt"), b);
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(VfsPath.of("/a.txt"), VfsPath.of("/b.txt")));

        /* Both files should still exist with original contents */
        assertThat(impl.getFile(VfsPath.of("/a.txt"))).isEqualTo(a);
        assertThat(impl.getFile(VfsPath.of("/b.txt"))).isEqualTo(b);
    }
}
