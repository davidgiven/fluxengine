package com.cowlark.fluxengine.vfs;

import java.io.IOException;
import java.nio.file.FileStore;
import java.nio.file.FileSystem;
import java.nio.file.Path;
import java.nio.file.PathMatcher;
import java.nio.file.WatchService;
import java.nio.file.attribute.UserPrincipalLookupService;
import java.nio.file.spi.FileSystemProvider;
import java.util.Collections;
import java.util.Set;

public class FloppyFileSystem extends FileSystem implements AutoCloseable
{
    private final FloppyFileSystemProvider provider;
    private final int hardwareHandle; // Your hardware interface
    private boolean open = true;

    // Custom constructor accepting direct hardware dependencies
    public FloppyFileSystem(int hardwareHandle) throws IOException
    {
        this.hardwareHandle = hardwareHandle;
        this.provider = new FloppyFileSystemProvider(this);

        // Perform direct hardware initialization here
        //        this.hardwareHandle.mount();
    }

    public int getHardwareHandle()
    {
        return hardwareHandle;
    }

    @Override
    public FileSystemProvider provider()
    {
        return provider;
    }

    @Override
    public void close() throws IOException
    {
        if (open)
        {
            //            hardwareHandle.unmount();
            open = false;
        }
    }

    @Override
    public boolean isOpen()
    {
        return open;
    }

    @Override
    public boolean isReadOnly()
    {
        return false;
    }

    @Override
    public String getSeparator()
    {
        return "/";
    }

    @Override
    public Iterable<Path> getRootDirectories()
    {
        return Collections.singletonList(getPath("/"));
    }

    @Override
    public Iterable<FileStore> getFileStores()
    {
        return Collections.emptyList();
    }

    @Override
    public Set<String> supportedFileAttributeViews()
    {
        return Collections.singleton("basic");
    }

    @Override
    public Path getPath(String first, String... more)
    {
        String fullPath = first;
        if (more.length > 0)
        {
            fullPath = fullPath + "/" + String.join("/", more);
        }
        return new FloppyPath(this, fullPath);
    }

    @Override
    public PathMatcher getPathMatcher(String syntaxAndPattern)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public UserPrincipalLookupService getUserPrincipalLookupService()
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public WatchService newWatchService()
    {
        throw new UnsupportedOperationException();
    }

}