package com.cowlark.fluxengine.gui;

import com.cowlark.fluxengine.vfs.FileAttributes;
import com.cowlark.fluxengine.vfs.Filesystem;
import com.cowlark.fluxengine.vfs.FilesystemAttributes;
import com.cowlark.fluxengine.vfs.VfsPath;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.util.HashMap;
import java.util.Map;

/**
 * Minimal in-memory fake filesystem for headless GUI tests.
 *
 * <p>Backed by two maps: {@code dirents} for every path and {@code children}
 * for directory listings. Only the operations needed by {@code
 * FilesystemTreeTableModel} / {@code FilesystemPanel} are implemented.
 */
class FakeFilesystem extends Filesystem
{
    private final Map<VfsPath, Dirent> dirents = new HashMap<>();
    private final Map<VfsPath, Map<String, Dirent>> children = new HashMap<>();
    private boolean dirty = false;

    private static final ImmutableSet<Capability> CAPS = ImmutableSet.of(
            Capability.OP_LIST,
            Capability.OP_GETDIRENT,
            Capability.OP_CREATEDIR,
            Capability.OP_DELETE,
            Capability.OP_GETFILE,
            Capability.OP_PUTFILE,
            Capability.OP_MOVE,
            Capability.OP_GETFSDATA);

    FakeFilesystem()
    {
        super(CAPS);
        /* Root directory. */
        Dirent root = Dirent.builder()
                .setPath(VfsPath.of("/"))
                .setFilename("/")
                .setFileType(FileType.IS_DIR)
                .setAttributes(ImmutableMap.of(
                        FileAttributes.FILENAME.name(), "/",
                        FileAttributes.FILE_TYPE.name(), "dir"))
                .build();
        dirents.put(VfsPath.of("/"), root);
        children.put(VfsPath.of("/"), new HashMap<>());
    }

    void addEmptyDir(VfsPath path)
    {
        String name = path.getName();
        Dirent d = Dirent.builder()
                .setPath(path)
                .setFilename(name)
                .setFileType(FileType.IS_DIR)
                .setAttributes(ImmutableMap.of(
                        FileAttributes.FILENAME.name(), name,
                        FileAttributes.FILE_TYPE.name(), "dir"))
                .build();
        dirents.put(path, d);
        children.put(path, new HashMap<>());
        VfsPath parent = path.getParent();
        if (parent == null) parent = VfsPath.of("/");
        children.get(parent).put(name, d);
    }

    void addFile(VfsPath path)
    {
        String name = path.getName();
        Dirent d = Dirent.builder()
                .setPath(path)
                .setFilename(name)
                .setFileType(FileType.IS_FILE)
                .setAttributes(ImmutableMap.of(
                        FileAttributes.FILENAME.name(), name,
                        FileAttributes.FILE_TYPE.name(), "file"))
                .build();
        dirents.put(path, d);
        VfsPath parent = path.getParent();
        if (parent == null) parent = VfsPath.of("/");
        Map<String, Dirent> pChildren = children.get(parent);
        if (pChildren == null) {
            pChildren = new HashMap<>();
            children.put(parent, pChildren);
        }
        pChildren.put(name, d);
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        Map<String, Dirent> m = children.get(path);
        if (m == null) throw new IOException("not a directory: " + path);
        return ImmutableMap.copyOf(m);
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        Dirent d = dirents.get(path);
        if (d == null) throw new IOException("not found: " + path);
        return d;
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        if (dirents.containsKey(path)) throw new IOException("exists: " + path);
        addEmptyDir(path);
        dirty = true;
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        Dirent d = dirents.remove(path);
        if (d == null) throw new IOException("not found: " + path);
        /* Remove from parent. */
        VfsPath parent = path.getParent();
        if (parent == null) parent = VfsPath.of("/");
        Map<String, Dirent> pChildren = children.get(parent);
        if (pChildren != null) pChildren.remove(path.getName());
        /* If directory, remove its children map. */
        children.remove(path);
        dirty = true;
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        return ImmutableMap.of(
                FilesystemAttributes.BLOCK_SIZE.name(), "512",
                FilesystemAttributes.TOTAL_BLOCKS.name(), "100",
                FilesystemAttributes.USED_BLOCKS.name(), dirty ? "10" : "5");
    }

    @Override
    public boolean needsFlushing()
    {
        return dirty;
    }

    @Override
    public void flushChanges() throws IOException
    {
        dirty = false;
    }

    @Override
    public void discardChanges() throws IOException
    {
        dirty = false;
        /*
         * For this fake, discard does not revert structure.
         * Tests that need revert can snapshot and restore manually.
         * For the empty directory bug, this is not needed.
         */
    }

    @Override
    public boolean isReadOnly()
    {
        return false;
    }
}
