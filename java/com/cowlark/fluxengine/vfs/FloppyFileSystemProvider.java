package com.cowlark.fluxengine.vfs;

import java.io.IOException;
import java.net.URI;
import java.nio.channels.SeekableByteChannel;
import java.nio.file.AccessMode;
import java.nio.file.CopyOption;
import java.nio.file.DirectoryStream;
import java.nio.file.FileStore;
import java.nio.file.FileSystem;
import java.nio.file.LinkOption;
import java.nio.file.OpenOption;
import java.nio.file.Path;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.attribute.FileAttribute;
import java.nio.file.attribute.FileAttributeView;
import java.nio.file.spi.FileSystemProvider;
import java.util.Map;
import java.util.Set;

public class FloppyFileSystemProvider extends FileSystemProvider
{
    private final FloppyFileSystem fileSystem;

    public FloppyFileSystemProvider(FloppyFileSystem fileSystem)
    {
        this.fileSystem = fileSystem;
    }

    @Override
    public String getScheme()
    {
        // Not registered with SPI, scheme is arbitrary
        return "hw";
    }

    @Override
    public FileSystem newFileSystem(URI uri, Map<String, ?> env)
    {
        throw new UnsupportedOperationException("Instantiate HardwareFileSystem directly instead.");
    }

    @Override
    public FileSystem getFileSystem(URI uri)
    {
        return fileSystem;
    }

    @Override
    public Path getPath(URI uri)
    {
        return fileSystem.getPath(uri.getPath());
    }

    @Override
    public SeekableByteChannel newByteChannel(
            Path path,
            Set<? extends OpenOption> options,
            FileAttribute<?>... attrs) throws IOException
    {
        FloppyPath hwPath = (FloppyPath) path;
        // Delegate directly to your hardware layer channel implementation
        //        return hwPath.getFileSystem().getHardwareHandle().openChannel(hwPath.toString()
        //        , options);
        throw new UnsupportedOperationException();
    }

    @Override
    public DirectoryStream<Path> newDirectoryStream(
            Path dir,
            DirectoryStream.Filter<? super Path> filter) throws IOException
    {
        // Implement hardware directory iteration
        throw new UnsupportedOperationException();
    }

    @Override
    public void createDirectory(Path dir, FileAttribute<?>... attrs) throws IOException
    {
        //        ((FloppyPath) dir).getFileSystem().getHardwareHandle().mkdir(dir.toString());
        throw new UnsupportedOperationException();
    }

    @Override
    public void delete(Path path) throws IOException
    {
        //        ((FloppyPath) path).getFileSystem().getHardwareHandle().delete(path.toString());
        throw new UnsupportedOperationException();
    }

    @Override
    public void copy(Path source, Path target, CopyOption... options) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public void move(Path source, Path target, CopyOption... options) throws IOException
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public boolean isSameFile(Path path, Path path2)
    {
        return path.toAbsolutePath().toString().equals(path2.toAbsolutePath().toString());
    }

    @Override
    public boolean isHidden(Path path)
    {
        return false;
    }

    @Override
    public FileStore getFileStore(Path path)
    {
        return null;
    }

    @Override
    public void checkAccess(Path path, AccessMode... modes) throws IOException
    {
        // Check existence on hardware
    }

    @Override
    public <V extends FileAttributeView> V getFileAttributeView(
            Path path,
            Class<V> type,
            LinkOption... options)
    {
        return null;
    }

    @Override
    @SuppressWarnings("unchecked")
    public <A extends BasicFileAttributes> A readAttributes(
            Path path,
            Class<A> type,
            LinkOption... options) throws IOException
    {
        //        if (type == BasicFileAttributes.class)
        //        {
        //            // Fetch hardware metadata
        //            return (A) ((FloppyPath) path)
        //                    .getFileSystem()
        //                    .getHardwareHandle()
        //                    .getAttributes(path.toString());
        //        }
        throw new UnsupportedOperationException();
    }

    @Override
    public Map<String, Object> readAttributes(Path path, String attributes, LinkOption... options)
    {
        throw new UnsupportedOperationException();
    }

    @Override
    public void setAttribute(Path path, String attribute, Object value, LinkOption... options)
    {
        throw new UnsupportedOperationException();
    }
}
