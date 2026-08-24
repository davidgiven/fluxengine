package com.cowlark.fluxengine.vfs;

import java.io.File;
import java.net.URI;
import java.nio.file.LinkOption;
import java.nio.file.Path;
import java.nio.file.WatchEvent;
import java.nio.file.WatchKey;
import java.nio.file.WatchService;

public class FloppyPath implements Path
{
    private final FloppyFileSystem fileSystem;
    private final String path;

    public FloppyPath(FloppyFileSystem fileSystem, String path)
    {
        this.fileSystem = fileSystem;
        this.path = path;
    }

    @Override
    public FloppyFileSystem getFileSystem()
    {
        return fileSystem;
    }

    @Override
    public boolean isAbsolute()
    {
        return path.startsWith("/");
    }

    @Override
    public Path getRoot()
    {
        return isAbsolute() ? fileSystem.getPath("/") : null;
    }

    @Override
    public Path getFileName()
    {
        int idx = path.lastIndexOf('/');
        return idx == -1 ? this : fileSystem.getPath(path.substring(idx + 1));
    }

    @Override
    public Path getParent()
    {
        int idx = path.lastIndexOf('/');
        if (idx <= 0)
            return idx == 0 ? getRoot() : null;
        return fileSystem.getPath(path.substring(0, idx));
    }

    @Override
    public int getNameCount()
    {
        return (int) java.util.Arrays.stream(path.split("/")).filter(s -> !s.isEmpty()).count();
    }

    @Override
    public Path getName(int index)
    {
        String[] parts = java.util.Arrays
                .stream(path.split("/"))
                .filter(s -> !s.isEmpty())
                .toArray(String[]::new);
        return fileSystem.getPath(parts[index]);
    }

    @Override
    public Path subpath(int beginIndex, int endIndex)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean startsWith(Path other)
    {
        return this.path.startsWith(other.toString());
    }

    @Override
    public boolean endsWith(Path other)
    {
        return this.path.endsWith(other.toString());
    }

    @Override
    public Path normalize()
    {
        return this;
    }

    @Override
    public Path resolve(Path other)
    {
        if (other.isAbsolute())
            return other;
        return fileSystem.getPath(this.path + "/" + other.toString());
    }

    @Override
    public Path relativize(Path other)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public URI toUri()
    {
        // Safe to throw if URI resolution is not desired
        throw new UnsupportedOperationException("URI resolution disabled for hardware VFS");
    }

    @Override
    public Path toAbsolutePath()
    {
        return isAbsolute() ? this : fileSystem.getPath("/" + path);
    }

    @Override
    public Path toRealPath(LinkOption... options)
    {
        return toAbsolutePath();
    }

    @Override
    public File toFile()
    {
        throw new UnsupportedOperationException("Not an OS file system");
    }

    @Override
    public WatchKey register(
            WatchService watcher,
            WatchEvent.Kind<?>[] events,
            WatchEvent.Modifier... modifiers)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public int compareTo(Path other)
    {
        return this.path.compareTo(other.toString());
    }

    @Override
    public String toString()
    {
        return path;
    }
}