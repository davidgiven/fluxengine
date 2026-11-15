package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import lombok.SneakyThrows;
import org.rob_smallshire.oaknut.dfs.Dfs;
import org.rob_smallshire.oaknut.dfs.DfsDiscInfo;
import org.rob_smallshire.oaknut.dfs.DfsFileEntry;
import org.rob_smallshire.oaknut.dfs.DfsFileLockedException;
import org.rob_smallshire.oaknut.dfs.DfsFormat;
import org.rob_smallshire.oaknut.dfs.DfsReadOnlyException;
import org.rob_smallshire.oaknut.file.Access;
import org.rob_smallshire.oaknut.file.AcornMeta;
import java.io.IOException;
import java.nio.file.AccessDeniedException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;

public class AcornDfsFilesystem extends Filesystem
{
    public static final String ACORNDFS_LOAD_ADDRESS = "acorndfs.load_address";
    public static final String ACORNDFS_EXEC_ADDRESS = "acorndfs.exec_address";

    /* DFS volumes have one flat catalogue. Directory letters belong to filenames. */
    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of(
            Capability.OP_CREATE,
            Capability.OP_CHECK,
            Capability.OP_LIST,
            Capability.OP_GETFILE,
            Capability.OP_PUTFILE,
            Capability.OP_GETDIRENT,
            Capability.OP_DELETE,
            Capability.OP_MOVE,
            Capability.OP_GETFSDATA,
            Capability.OP_PUTFSDATA,
            Capability.OP_PUTATTRS);

    private final AcornDfsProto config;
    private final BlockDevice blockDevice;
    private Dfs dfs;

    public AcornDfsFilesystem(AcornDfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    /* All VFS file paths contain one component. Oaknut accepts both bare names and A.NAME;
     * a stored $.NAME is exposed as NAME, while other prefixes are retained literally. */
    private static String name(VfsPath path)
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(
                    path.toString(),
                    "DFS filenames must have one path component");
        return path.getName();
    }

    private static Dirent makeDirent(DfsFileEntry entry)
    {
        String filename = entry.name().startsWith("$.") ? entry.name().substring(2) : entry.name();
        String mode = Access.format(entry.metadata().access());
        return Dirent
                .builder()
                .setFilename(filename)
                .setPath(VfsPath.root().resolve(filename))
                .setFileType(FileType.IS_FILE)
                .setLength(entry.length())
                .setMode(mode)
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(FileAttributes.FILENAME.name(), filename)
                        .put(FileAttributes.FILE_TYPE.name(), "file")
                        .put(FileAttributes.LENGTH.name(), Integer.toString(entry.length()))
                        .put(FileAttributes.MODE.name(), mode)
                        .put(ACORNDFS_LOAD_ADDRESS, String.format("0x%x", entry.loadAddress()))
                        .put(ACORNDFS_EXEC_ADDRESS, String.format("0x%x", entry.executionAddress()))
                        .build())
                .build();
    }

    /* BlockDevice already supplies logical sector order and selects the DFS volume's side. */
    private DfsFormat format() throws IOException
    {
        boolean watford = switch (config.getFlavour())
        {
            case ACORN_DFS -> false;
            case WATCOM_DFS -> true;
            default -> throw new IllegalArgumentException("DFS flavour is undefined");
        };
        if (blockDevice.getBlockSize() != 256)
            throw new IOException("DFS requires 256-byte sectors");
        DfsFormat format = switch (blockDevice.getBlockCount())
        {
            case 400 -> DfsFormat.ACORN_DFS_40T_SINGLE_SIDED;
            case 800 -> DfsFormat.ACORN_DFS_80T_SINGLE_SIDED;
            default -> throw new IOException("Unsupported DFS volume capacity");
        };
        return format.withWatford(watford);
    }

    /* Share ADFS's write-through adapter. Oaknut snapshots are disabled because BlockDevice
     * owns caching and rollback; mounting reads the catalogue rather than the whole volume. */
    private void mount() throws IOException
    {
        if (dfs == null)
            dfs = Dfs.fromStorage(new OaknutBlockDeviceStorage(blockDevice), format(), false);
    }

    private void dismount() throws IOException
    {
        if (dfs == null)
            return;
        try
        {
            dfs.close();
        } finally
        {
            dfs = null;
        }
    }

    private <T> T access(VfsPath path, IoOperation<T> operation) throws IOException
    {
        try
        {
            mount();
            return operation.run();
        } catch (DfsFileLockedException | DfsReadOnlyException e)
        {
            throw new AccessDeniedException(path.toString(), null, e.getMessage());
        } catch (FileAlreadyExistsException e)
        {
            throw new FileAlreadyExistsException(path.toString(), null, e.getMessage());
        } catch (NoSuchFileException e)
        {
            throw new NoSuchFileException(path.toString(), null, e.getMessage());
        } catch (IllegalArgumentException e)
        {
            throw new InvalidPathException(path.toString(), e.getMessage());
        }
    }

    /* Formatting does not mount or read unformatted sectors. Quick format replaces the
     * catalogue and clears stale Watford markers; full format initializes the entire volume. */
    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        try (Dfs blank = Dfs.create(format(), volumeName))
        {
            Bytes image = new Bytes(blank.toByteArray());
            dismount();
            blockDevice.putBytes(0, quick ? image.slice(0, 1024) : image);
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
                VfsPath.root(), () -> {
                    DfsDiscInfo info = dfs.info();
                    int total = Math.min(info.totalSectors(), blockDevice.getBlockCount());
                    return ImmutableMap.of(
                            FilesystemAttributes.VOLUME_NAME.name(),
                            info.title(),
                            FilesystemAttributes.TOTAL_BLOCKS.name(),
                            Integer.toString(total),
                            FilesystemAttributes.USED_BLOCKS.name(),
                            Integer.toString(total - dfs.freeSectors()),
                            FilesystemAttributes.BLOCK_SIZE.name(),
                            "256");
                });
    }

    @Override
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata) throws IOException
    {
        if (!metadata.keySet().equals(ImmutableSet.of(FilesystemAttributes.VOLUME_NAME.name())))
            throw new IllegalArgumentException("can't set this metadata key");
        access(
                VfsPath.root(), () -> {
                    dfs.setTitle(metadata.get(FilesystemAttributes.VOLUME_NAME.name()));
                    return null;
                });
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());
        return access(
                path, () -> {
                    ImmutableMap.Builder<String, Dirent> entries = ImmutableMap.builder();
                    for (DfsFileEntry entry : dfs.files())
                    {
                        Dirent dirent = makeDirent(entry);
                        entries.put(dirent.filename(), dirent);
                    }
                    return entries.build();
                });
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        return access(path, () -> new Bytes(dfs.readFile(name(path))));
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        access(
                path, () -> {
                    String filename = name(path);
                    AcornMeta metadata = dfs.exists(filename) ?
                            dfs.stat(filename).metadata() :
                            new AcornMeta(0, 0, null);
                    dfs.writeFile(filename, bytes.toByteArray(), metadata);
                    return null;
                });
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        return access(path, () -> path.isRoot() ? ROOT_DIRENT : makeDirent(dfs.stat(name(path))));
    }

    @Override
    @SneakyThrows(IOException.class)
    public void putFileMetadata(VfsPath path, ImmutableMap<String, String> metadata)
    {
        if (!ImmutableSet
                .of(FileAttributes.MODE.name(), ACORNDFS_LOAD_ADDRESS, ACORNDFS_EXEC_ADDRESS)
                .containsAll(metadata.keySet()))
            throw new IllegalArgumentException("can't set this metadata key");
        access(
                path, () -> {
                    String filename = name(path);
                    AcornMeta previous = dfs.stat(filename).metadata();
                    dfs.setMetadata(
                            filename, new AcornMeta(
                                    metadata.containsKey(ACORNDFS_LOAD_ADDRESS) ?
                                            Long.decode(metadata.get(ACORNDFS_LOAD_ADDRESS)) :
                                            previous.loadAddress(),
                                    metadata.containsKey(ACORNDFS_EXEC_ADDRESS) ?
                                            Long.decode(metadata.get(ACORNDFS_EXEC_ADDRESS)) :
                                            previous.executionAddress(),
                                    metadata.containsKey(FileAttributes.MODE.name()) ?
                                            Access.parse(metadata.get(FileAttributes.MODE.name())) :
                                            previous.access()));
                    return null;
                });
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        access(
                path, () -> {
                    dfs.deleteFile(name(path));
                    return null;
                });
    }

    @Override
    public void moveFile(VfsPath source, VfsPath destination) throws IOException
    {
        access(
                source, () -> {
                    dfs.renameFile(name(source), name(destination));
                    return null;
                });
    }

    /* Commit/revert invalidates the mounted handle, matching ADFS and the other adapters. */
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

    /* Translate oaknut failures into the exceptions expected by VFS clients. */
    @FunctionalInterface
    private interface IoOperation<T>
    {
        T run() throws IOException;
    }
}
