package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import lombok.SneakyThrows;
import org.rob_smallshire.oaknut.adfs.Adfs;
import org.rob_smallshire.oaknut.adfs.AdfsDirectoryNotEmptyException;
import org.rob_smallshire.oaknut.adfs.AdfsEntryExistsException;
import org.rob_smallshire.oaknut.adfs.AdfsFileLockedException;
import org.rob_smallshire.oaknut.adfs.AdfsFormat;
import org.rob_smallshire.oaknut.adfs.AdfsPath;
import org.rob_smallshire.oaknut.adfs.AdfsPathException;
import org.rob_smallshire.oaknut.adfs.AdfsReadOnlyException;
import org.rob_smallshire.oaknut.adfs.AdfsStat;
import org.rob_smallshire.oaknut.adfs.DiscRecord;
import org.rob_smallshire.oaknut.discimage.Storage;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import java.io.IOException;
import java.io.UncheckedIOException;
import java.nio.file.AccessDeniedException;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.time.Instant;
import java.util.Objects;

public class AcornAdfsFilesystem extends Filesystem
{
    /* All paths at the VFS boundary use slash-separated components. */
    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of(
            Capability.OP_CREATE,
            Capability.OP_CHECK,
            Capability.OP_LIST,
            Capability.OP_GETFILE,
            Capability.OP_PUTFILE,
            Capability.OP_GETDIRENT,
            Capability.OP_CREATEDIR,
            Capability.OP_DELETE,
            Capability.OP_MOVE,
            Capability.OP_GETFSDATA,
            Capability.OP_PUTFSDATA,
            Capability.OP_PUTATTRS);

    private final AcornAdfsProto config;
    private final BlockDevice blockDevice;
    private Adfs adfs;

    public AcornAdfsFilesystem(AcornAdfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private static Dirent makeDirent(VfsPath path, AdfsStat stat)
    {
        ImmutableMap.Builder<String, String> attributes = ImmutableMap.builder();
        attributes.put(FileAttributes.FILENAME.name(), stat.name());
        attributes.put(FileAttributes.FILE_TYPE.name(), stat.directory() ? "dir" : "file");
        String mode = Access.format(stat.access());
        attributes.put(FileAttributes.MODE.name(), mode);
        Dirent.DirentBuilder builder = Dirent
                .builder()
                .setFilename(stat.name())
                .setPath(path)
                .setFileType(stat.directory() ? FileType.IS_DIR : FileType.IS_FILE)
                .setMode(mode);
        if (!stat.directory())
        {
            builder.setLength(stat.length());
            attributes.put(FileAttributes.LENGTH.name(), Integer.toString(stat.length()));
        }
        Instant time = stat.metadata().datestamp();
        if (time != null)
            attributes.put(FileAttributes.CTIME.name(), time.toString());
        return builder.setAttributes(attributes.build()).build();
    }

    /* Oaknut reads blocks on demand. BlockDevice supplies rollback, so oaknut snapshots are off. */
    private void mount() throws IOException
    {
        if (adfs != null)
            return;

        adfs = Adfs.fromStorage(new OaknutBlockDeviceStorage(blockDevice), false);
    }

    /* The storage is borrowed; closing oaknut does not commit or close the block device. */
    private void dismount() throws IOException
    {
        if (adfs == null)
            return;

        try
        {
            adfs.close();
        } finally
        {
            adfs = null;
        }
    }

    /* Lifecycle operations mirror the other filesystem adapters and invalidate the mounted
    handle. */
    @Override
    public boolean needsFlushing()
    {
        return blockDevice.needsCommit();
    }

    @Override
    public void flushChanges() throws IOException
    {
        blockDevice.commit();
        dismount();
    }

    @Override
    public void discardChanges() throws IOException
    {
        try
        {
            dismount();
        } finally
        {
            blockDevice.revert();
        }
    }

    @Override
    public void close() throws IOException
    {
        flushChanges();
    }

    /* Keep oaknut names as components, including literal dots and Acorn special tokens. */
    private AdfsPath path(VfsPath path)
    {
        return adfs.path(path.segments());
    }

    private void validateName(VfsPath path)
    {
        if (path.isRoot())
            throw new InvalidPathException(path.toString(), "cannot replace the root directory");
        adfs.format().directoryFormat.validateName(path.getName());
    }

    private <T> T access(VfsPath path, IoOperation<T> operation) throws IOException
    {
        try
        {
            mount();
            return operation.run();
        } catch (UncheckedIOException e)
        {
            throw e.getCause();
        } catch (AdfsEntryExistsException e)
        {
            throw new FileAlreadyExistsException(path.toString(), null, e.getMessage());
        } catch (AdfsDirectoryNotEmptyException e)
        {
            throw new DirectoryNotEmptyException(path.toString());
        } catch (AdfsFileLockedException | AdfsReadOnlyException e)
        {
            throw new AccessDeniedException(path.toString(), null, e.getMessage());
        } catch (AdfsPathException e)
        {
            throw new NoSuchFileException(path.toString(), null, e.getMessage());
        } catch (IllegalArgumentException e)
        {
            throw new InvalidPathException(path.toString(), e.getMessage());
        }
    }

    /* Capacity determines the default floppy shape; existing images retain their map format
     * when opened. Ambiguous 800K blank disks are formatted as E rather than D. */
    private AdfsFormat creationFormat() throws IOException
    {
        int size = Math.multiplyExact(blockDevice.getBlockSize(), blockDevice.getBlockCount());
        return switch (size)
        {
            case 163840 -> AdfsFormat.S;
            case 327680 -> AdfsFormat.M;
            case 655360 -> AdfsFormat.L;
            case 819200 -> AdfsFormat.E;
            case 1638400 -> AdfsFormat.F;
            case 3276800 -> AdfsFormat.G;
            default -> throw new IOException("Unsupported ADFS floppy capacity: " + size);
        };
    }

    /* Whole-block metadata writes allow quick formatting without reading unformatted sectors. */
    private void writeRegion(Bytes image, int offset, int length) throws IOException
    {
        int blockSize = blockDevice.getBlockSize();
        int first = offset / blockSize * blockSize;
        int end = (offset + length + blockSize - 1) / blockSize * blockSize;
        blockDevice.putBytes(first, image.slice(first, end - first));
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        AdfsFormat format = creationFormat();
        try (Adfs blank = Adfs.create(format, volumeName))
        {
            Bytes image = new Bytes(blank.toByteArray());
            dismount();
            if (!quick)
                blockDevice.putBytes(0, image);
            else if (!format.newMap())
                writeRegion(
                        image,
                        0,
                        (format == AdfsFormat.D ? 1024 : 512) + format.directoryFormat.size());
            else
            {
                DiscRecord record = DiscRecord.forFormat(format, "", 0, 0);
                if (record.zones() > 1)
                    writeRegion(image, 0xc00, 512);
                writeRegion(
                        image,
                        record.bootmap(),
                        record.zones() * record.sectorSize() * 2 + format.directoryFormat.size());
            }
        } catch (IllegalArgumentException e)
        {
            throw new InvalidPathException(volumeName, e.getMessage());
        }
    }

    @Override
    @SneakyThrows(IOException.class)
    public void check()
    {
        for (int i = 0; i < blockDevice.getBlockCount(); i++)
        {
            Bytes block = blockDevice.getBlock(i);
            if (block == null || block.size() != blockDevice.getBlockSize())
                throw new IOException("Missing or short block: " + i);
        }
    }

    @Override
    public boolean isReadOnly()
    {
        return false;
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        return access(
                VfsPath.root(), () -> ImmutableMap.of(
                        FilesystemAttributes.VOLUME_NAME.name(),
                        adfs.title(),
                        FilesystemAttributes.TOTAL_BLOCKS.name(),
                        Integer.toString(adfs.totalSize() / 256),
                        FilesystemAttributes.USED_BLOCKS.name(),
                        Integer.toString((adfs.totalSize() - adfs.freeSpace()) / 256),
                        FilesystemAttributes.BLOCK_SIZE.name(),
                        "256"));
    }

    @Override
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata) throws IOException
    {
        if (!metadata.keySet().equals(ImmutableSet.of(FilesystemAttributes.VOLUME_NAME.name())))
            throw new IllegalArgumentException("can't set this metadata key");
        access(
                VfsPath.root(), () -> {
                    adfs.setTitle(metadata.get(FilesystemAttributes.VOLUME_NAME.name()));
                    return null;
                });
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath directory) throws IOException
    {
        return access(
                directory, () -> {
                    ImmutableMap.Builder<String, Dirent> entries = ImmutableMap.builder();
                    for (AdfsPath child : path(directory).list())
                        entries.put(
                                child.name(),
                                makeDirent(directory.resolve(child.name()), child.stat()));
                    return entries.build();
                });
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        return access(path, () -> new Bytes(path(path).readBytes()));
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        access(
                path, () -> {
                    validateName(path);
                    AdfsPath target = path(path);
                    if (target.isDirectory())
                        throw new FileAlreadyExistsException(path.toString());
                    if (target.exists())
                        target.writeBytes(bytes.toByteArray(), target.stat().metadata());
                    else
                        target.writeBytes(bytes.toByteArray());
                    return null;
                });
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        return access(
                path, () -> {
                    if (path.isRoot())
                        return ROOT_DIRENT;
                    AdfsStat stat = path(path).stat();
                    return makeDirent(path.getParent().resolve(stat.name()), stat);
                });
    }

    @Override
    @SneakyThrows(IOException.class)
    public void putFileMetadata(VfsPath path, ImmutableMap<String, String> metadata)
    {
        if (!ImmutableSet
                .of(FileAttributes.MODE.name(), FileAttributes.CTIME.name())
                .containsAll(metadata.keySet()))
            throw new IllegalArgumentException("can't set this metadata key");
        access(
                path, () -> {
                    AdfsPath file = path(path);
                    AcornMeta previous = file.stat().metadata();
                    AcornMeta updated = new AcornMeta(
                            previous.loadAddress(),
                            previous.executionAddress(),
                            metadata.containsKey(FileAttributes.MODE.name()) ?
                                    Access.parse(metadata.get(FileAttributes.MODE.name())) :
                                    previous.access());
                    if (metadata.containsKey(FileAttributes.CTIME.name()))
                        updated =
                                updated.withDatestamp(Instant.parse(metadata.get(FileAttributes.CTIME.name())));
                    file.setMetadata(updated);
                    return null;
                });
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        access(
                path, () -> {
                    validateName(path);
                    path(path).mkdir();
                    return null;
                });
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        access(
                path, () -> {
                    if (path.isRoot())
                        throw new AccessDeniedException(
                                path.toString(),
                                null,
                                "cannot delete root");
                    AdfsPath target = path(path);
                    if (target.stat().directory())
                        target.rmdir();
                    else
                        target.unlink();
                    return null;
                });
    }

    @Override
    public void moveFile(VfsPath source, VfsPath destination) throws IOException
    {
        access(
                source, () -> {
                    if (source.isRoot() || source.equals(destination) ||
                            (destination.segments().size() > source.segments().size() && destination
                                    .segments()
                                    .subList(0, source.segments().size())
                                    .equals(source.segments()) && path(source).isDirectory()))
                        throw new InvalidPathException(
                                destination.toString(),
                                "cannot move a directory into itself");
                    validateName(destination);
                    path(source).rename(path(destination));
                    return null;
                });
    }

    /* Translate library errors at the boundary so generic VFS clients see standard exceptions. */
    @FunctionalInterface
    private interface IoOperation<T>
    {
        T run() throws IOException;
    }

    /* Delegate byte ranges directly to BlockDevice, which owns caching and pending changes.
     * Committing and reverting remain the filesystem adapter's responsibility. */
    static final class OaknutBlockDeviceStorage implements Storage
    {
        private final BlockDevice device;
        private final int size;

        OaknutBlockDeviceStorage(BlockDevice device)
        {
            this.device = device;
            size = Math.multiplyExact(device.getBlockSize(), device.getBlockCount());
        }

        @Override
        public int size()
        {
            return size;
        }

        @Override
        public void read(int offset, byte[] destination, int destinationOffset, int length)
                throws IOException
        {
            Objects.checkFromIndexSize(offset, length, size);
            Objects.checkFromIndexSize(destinationOffset, length, destination.length);
            byte[] bytes = device.getBytes(offset, length).toByteArray();
            System.arraycopy(bytes, 0, destination, destinationOffset, length);
        }

        @Override
        public void write(int offset, byte[] source, int sourceOffset, int length)
                throws IOException
        {
            Objects.checkFromIndexSize(offset, length, size);
            Objects.checkFromIndexSize(sourceOffset, length, source.length);
            device.putBytes(offset, new Bytes(source).slice(sourceOffset, length));
        }
    }
}
