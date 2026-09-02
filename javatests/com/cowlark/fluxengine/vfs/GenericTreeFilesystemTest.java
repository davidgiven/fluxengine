package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.Filesystem.Capability;
import com.cowlark.fluxengine.vfs.Filesystem.Dirent;
import com.google.common.collect.ImmutableMap;
import org.junit.Assume;
import org.junit.Test;
import java.io.IOException;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;

public abstract class GenericTreeFilesystemTest extends GenericFilesystemTest
{
    private void assumeTree(Capability... caps)
    {
        for (Capability c : caps)
            Assume.assumeTrue(impl.getCapabilities().contains(c));
    }

    @Test
    public void putFile_onDirectory() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_CREATEDIR, Capability.OP_PUTFILE);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/data"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.putFile(VfsPath.of("/data"), getTestFileData()));
    }

    @Test
    public void createDirectory() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_CREATEDIR, Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir"));
        Dirent de = impl.getDirent(VfsPath.of("/dir"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void createDirectory_fileExists() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_PUTFILE, Capability.OP_CREATEDIR);
        impl.create(true, "LABEL");
        impl.putFile(VfsPath.of("/data"), getTestFileData());
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.createDirectory(VfsPath.of("/data")));
    }

    @Test
    public void createDirectory_nested() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_LIST,
                Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir1"));
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.list(VfsPath.of("/dir1"))).hasSize(0);

        impl.createDirectory(VfsPath.of("/dir1", "dir2"));
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.list(VfsPath.of("/dir1"))).hasSize(1);

        Dirent de = impl.getDirent(VfsPath.of("/dir1/dir2"));
        assertThat(de.filename()).isEqualTo("dir2");
        assertThat(de.fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void createDirectory_middleMissing() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_CREATEDIR);
        impl.create(true, "LABEL");
        assertThrows(
                NoSuchFileException.class,
                () -> impl.createDirectory(VfsPath.of("/dir1", "dir2", "dir3", "dir4")));
    }

    @Test
    public void delete_dir() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_DELETE,
                Capability.OP_LIST);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir"));
        impl.deleteFile(VfsPath.of("/dir"));
        assertThat(impl.list(VfsPath.of("/"))).isEmpty();
    }

    @Test
    public void delete_middle() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_CREATEDIR, Capability.OP_DELETE);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir1"));
        impl.createDirectory(VfsPath.of("/dir1", "dir2"));
        assertThrows(DirectoryNotEmptyException.class, () -> impl.deleteFile(VfsPath.of("/dir1")));
    }

    @Test
    public void listDirectories() throws IOException
    {
        assumeTree(Capability.OP_CREATE, Capability.OP_CREATEDIR, Capability.OP_LIST);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir"));
        ImmutableMap<String, Dirent> files = impl.list(VfsPath.of("/"));
        assertThat(files).hasSize(1);
        assertThat(files.get("dir")).isEqualTo(Dirent
                .builder()
                .setPath(VfsPath.of("/dir"))
                .setFilename("dir")
                .setFileType(IS_DIR)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "dir")
                        .put(Attributes.FILE_TYPE, "dir")
                        .build())
                .build());
    }

    @Test
    public void flushActuallyFlushes() throws IOException
    {
        assumeTree(
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
        assumeTree(
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
    public void moveDirectory() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_PUTFILE,
                Capability.OP_MOVE,
                Capability.OP_GETDIRENT,
                Capability.OP_GETFILE,
                Capability.OP_LIST);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir1"));
        Bytes data = getTestFileData("inside");
        impl.putFile(VfsPath.of("/dir1/file.txt"), data);
        impl.moveFile(VfsPath.of("/dir1"), VfsPath.of("/dir2"));
        assertThrows(NoSuchFileException.class, () -> impl.getDirent(VfsPath.of("/dir1")));

        Dirent de = impl.getDirent(VfsPath.of("/dir2"));
        assertThat(de.fileType()).isEqualTo(IS_DIR);
        assertThat(impl.getFile(VfsPath.of("/dir2/file.txt"))).isEqualTo(data);
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
    }

    @Test
    public void moveFileIntoDirectory() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_PUTFILE,
                Capability.OP_CREATEDIR,
                Capability.OP_MOVE,
                Capability.OP_GETFILE,
                Capability.OP_LIST);
        impl.create(true, "LABEL");
        Bytes data = getTestFileData("hello");
        impl.putFile(VfsPath.of("/file.txt"), data);
        impl.createDirectory(VfsPath.of("/dir"));
        impl.moveFile(VfsPath.of("/file.txt"), VfsPath.of("/dir/file.txt"));
        assertThrows(NoSuchFileException.class, () -> impl.getFile(VfsPath.of("/file.txt")));

        assertThat(impl.getFile(VfsPath.of("/dir/file.txt"))).isEqualTo(data);
        assertThat(impl.list(VfsPath.of("/"))).hasSize(1);
        assertThat(impl.list(VfsPath.of("/dir"))).hasSize(1);
    }

    @Test
    public void moveDirectoryIntoItself() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_CREATEDIR,
                Capability.OP_MOVE,
                Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        impl.createDirectory(VfsPath.of("/dir"));
        impl.createDirectory(VfsPath.of("/dir/sub"));

        /* Moving /dir into its own subtree should fail (e.g., /dir -> /dir/sub/moved) */
        assertThrows(
                InvalidPathException.class,
                () -> impl.moveFile(VfsPath.of("/dir"), VfsPath.of("/dir/sub/moved")));

        /* Also moving directly onto itself should fail */
        assertThrows(
                InvalidPathException.class,
                () -> impl.moveFile(VfsPath.of("/dir"), VfsPath.of("/dir")));

        /* Original still exists and is intact */
        assertThat(impl.getDirent(VfsPath.of("/dir")).fileType()).isEqualTo(IS_DIR);
        assertThat(impl.getDirent(VfsPath.of("/dir/sub")).fileType()).isEqualTo(IS_DIR);
    }

    @Test
    public void moveFileOnTopOfAnotherFile() throws IOException
    {
        assumeTree(
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

    @Test
    public void moveFileOnTopOfDirectory() throws IOException
    {
        assumeTree(
                Capability.OP_CREATE,
                Capability.OP_PUTFILE,
                Capability.OP_CREATEDIR,
                Capability.OP_MOVE,
                Capability.OP_GETFILE,
                Capability.OP_GETDIRENT);
        impl.create(true, "LABEL");
        Bytes data = getTestFileData("data");
        impl.putFile(VfsPath.of("/file.txt"), data);
        impl.createDirectory(VfsPath.of("/dir"));
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(VfsPath.of("/file.txt"), VfsPath.of("/dir")));

        /* Also moving a directory onto a file should fail */
        assertThrows(
                FileAlreadyExistsException.class,
                () -> impl.moveFile(VfsPath.of("/dir"), VfsPath.of("/file.txt")));
        assertThat(impl.getFile(VfsPath.of("/file.txt"))).isEqualTo(data);
        assertThat(impl.getDirent(VfsPath.of("/dir")).fileType()).isEqualTo(IS_DIR);
    }
}
