package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import lombok.Builder;
import java.io.IOException;
import java.nio.file.Path;

public abstract class FileSystemImpl implements AutoCloseable
{

    public enum Capability
    {
        OP_CREATE,
        OP_CHECK,
        OP_LIST,
        OP_GETFILE,
        OP_PUTFILE,
        OP_GETDIRENT,
        OP_CREATEDIR,
        OP_DELETE,
        OP_GETFSDATA,
        OP_PUTFSDATA,
        OP_PUTATTRS,
        OP_MOVE;
    }

    public enum FileType
    {
        IS_FILE, IS_DIR
    }

    @Builder(setterPrefix = "set")
    public record Dirent(Path path, String filename, int length, String mode, FileType fileType,
                         ImmutableMap<String, String> attributes)
    {
    }

    private final ImmutableSet<Capability> capabilities;

    protected FileSystemImpl(ImmutableSet<Capability> capabilities)
    {
        this.capabilities = capabilities;
    }

    @Override
    public void close() throws Exception
    {
    }

    /**
     * Retrieve capability information.
     */
    public ImmutableSet<Capability> getCapabilities()
    {
        return capabilities;
    }

    /**
     * Create a filesystem on the disk.
     */
    public void create(boolean quick, String volumeName) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Are all sectors on the filesystem present and good? (Does not check
     * filesystem consistency.)
     */
    public void check()
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Get volume metadata.
     */
    public ImmutableMap<String, String> getFilesystemMetadata()
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Update volume metadata.
     */
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata)
    {
        throw new UnsupportedOperationException();
    }

    /**
     * List files in a given directory.
     */
    public ImmutableMap<String, Dirent> list(Path path) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Read a file.
     */
    public Bytes getFile(Path path) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Write a file.
     */
    public void putFile(Path path, Bytes bytes) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Get a single file dirent (which includes the metadata).
     */
    public Dirent getDirent(Path path) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Update file metadata.
     */
    public void putFileMetadata(Path path, ImmutableMap<String, String> metadata)
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Creates a directory.
     */
    public void createDirectory(Path path) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Deletes a file or non-empty directory.
     */
    public void deleteFile(Path path) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Moves a file (including renaming it).
     */
    public void moveFile(Path oldName, Path newName)
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Is this filesystem's backing store read-only?
     */
    public boolean isReadOnly()
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Does this filesystem need flushing?
     */
    public boolean needsFlushing()
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Flushes any changes back to the disk.
     */
    public void flushChanges() throws IOException
    {
        throw new UnsupportedOperationException();
    }

    /**
     * Discards any pending changes.
     */
    public void discardChanges()
    {
        throw new UnsupportedOperationException();
    }
}
