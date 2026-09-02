package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_CREATE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_CREATEDIR;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_DELETE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_MOVE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_PUTFILE;
import static org.mars.hfsutils.HfsConstants.FK_DATA;
import static org.mars.hfsutils.HfsConstants.FK_RSRC;
import static org.mars.hfsutils.HfsConstants.HFS_BLOCKSZ;
import static org.mars.hfsutils.HfsConstants.HFS_ISDIR;
import static org.mars.hfsutils.HfsConstants.HFS_MODE_RDWR;

import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import lombok.SneakyThrows;
import org.mars.hfsutils.Hfs;
import org.mars.hfsutils.HfsDir;
import org.mars.hfsutils.HfsDirEnt;
import org.mars.hfsutils.HfsException;
import org.mars.hfsutils.HfsFileHandle;
import org.mars.hfsutils.HfsVol;
import org.mars.hfsutils.HfsVolEnt;
import org.mars.hfsutils.os.HfsOs;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;

public class MacHfsFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of(
            OP_CREATE,
            OP_LIST,
            OP_GETFILE,
            OP_PUTFILE,
            OP_GETDIRENT,
            OP_CREATEDIR,
            OP_DELETE,
            OP_MOVE,
            OP_GETFSDATA);

    private static class HfsOsAdapter extends HfsOs
    {
        private final BlockDevice underlying;
        private final String mountPath;
        int seekPos;

        HfsOsAdapter(BlockDevice underlying, String mountPath)
        {
            this.underlying = underlying;
            this.mountPath = mountPath;
        }

        @Override
        public int open(String path, int mode)
        {
            return 0;
        }

        @Override
        public int close()
        {
            return 0;
        }

        @Override
        public int same(String path)
        {
            return mountPath.equals(path) ? 1 : 0;
        }

        @Override
        public long seek(long offset)
        {
            int totalBlocks = underlying.getBlockCount();
            if (offset == -1)
                return totalBlocks;
            if (offset < 0 || offset > totalBlocks)
                return -1;
            seekPos = (int) offset;
            return offset;
        }

        @Override
        @SneakyThrows
        public long read(byte[] buf, long len)
        {
            Bytes data = underlying.getBlocks(seekPos, (int) len);
            byte[] src = data.toByteArray();
            System.arraycopy(src, 0, buf, 0, (int) len * HFS_BLOCKSZ);
            seekPos += len;
            return len;
        }

        @Override
        @SneakyThrows
        public long write(byte[] buf, long len)
        {
            byte[] toWrite = new byte[(int) len * HFS_BLOCKSZ];
            System.arraycopy(buf, 0, toWrite, 0, toWrite.length);
            Bytes data = new Bytes(toWrite);
            underlying.putBlocks(seekPos, data);
            seekPos += len;
            return len;
        }
    }

    private final BlockDevice blockDevice;
    private final MacHfsProto config;
    private final HfsOs hfsos;
    private final String mountPath;
    private HfsVol volume;

    public MacHfsFilesystem(BlockDevice blockDevice)
    {
        this(MacHfsProto.getDefaultInstance(), blockDevice);
    }

    public MacHfsFilesystem(MacHfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        this.mountPath = "vol-" + System.identityHashCode(blockDevice);
        this.hfsos = new HfsOsAdapter(blockDevice, mountPath);
        this.volume = null;
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        mount();

        HfsVolEnt ent = new HfsVolEnt();
        try
        {
            Hfs.hfsVstat(volume, ent);
            return ImmutableMap
                    .<String, String>builder()
                    .put(Attributes.VOLUME_NAME, charsToString(ent.name))
                    .put(Attributes.TOTAL_BLOCKS, Long.toString(ent.totbytes / HFS_BLOCKSZ))
                    .put(
                            Attributes.USED_BLOCKS,
                            Long.toString((ent.totbytes - ent.freebytes) / HFS_BLOCKSZ))
                    .put(Attributes.BLOCK_SIZE, "512")
                    .build();
        } catch (HfsException e)
        {
            throw mapHfsException(e, VfsPath.of("/"));
        }
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        unmount();

        try
        {
            Hfs.hfsFormat(hfsos, mountPath, 0, HFS_MODE_RDWR, volumeName, 0, null);
        } catch (HfsException e)
        {
            throw new IOException(e);
        }
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        mount();

        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        HfsDir dir;
        try
        {
            dir = Hfs.hfsOpendir(volume, pathToHfsPath(path));
            try
            {

                HfsDirEnt ent = new HfsDirEnt();
                while (true)
                {
                    try
                    {
                        Hfs.hfsReaddir(dir, ent);
                    } catch (HfsException e)
                    {
                        if (e.errno == HfsException.ENOENT)
                            break;
                        throw e;
                    }

                    String name = charsToString(ent.name);
                    VfsPath childPath = path.resolve(name);
                    builder.put(name, direntFromHfsEnt(childPath, ent));
                }

                return builder.build();
            } finally
            {
                Hfs.hfsClosedir(dir);
            }
        } catch (HfsException e)
        {
            throw mapHfsException(e, path);
        }
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        mount();

        try
        {
            String hfsPath = pathToHfsPath(path);
            HfsDirEnt ent = new HfsDirEnt();
            Hfs.hfsStat(volume, hfsPath, ent);
            HfsFileHandle fh = Hfs.hfsOpen(volume, hfsPath);
            try
            {
                AppleSingle as = new AppleSingle();
                as.type = new Bytes(new String(ent.uFile.type).getBytes(StandardCharsets.UTF_8));
                as.creator =
                        new Bytes(new String(ent.uFile.creator).getBytes(StandardCharsets.UTF_8));
                as.data = new Bytes(readFork(fh, FK_DATA, ent.uFile.dsize));
                as.rsrc = new Bytes(readFork(fh, FK_RSRC, ent.uFile.rsize));
                return as.render();
            } finally
            {
                Hfs.hfsClose(fh);
            }
        } catch (HfsException e)
        {
            throw mapHfsException(e, path);
        }
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        mount();
        String hfsPath = pathToHfsPath(path);

        AppleSingle as = new AppleSingle();
        try
        {
            as.parse(bytes);
        } catch (AppleSingle.InvalidFileException e)
        {
            throw new IOException("you can only write valid AppleSingle encoded files", e);
        }

        try
        {
            Hfs.hfsDelete(volume, hfsPath);
        } catch (HfsException e)
        { /* ignore */}

        HfsException lastEx = null;
        for (int attempt = 0; attempt < 2; attempt++)
        {
            try
            {
                String type = "    ";
                String creator = "    ";
                if (as.type.size() != 0)
                    type = new String(as.type.toByteArray(), StandardCharsets.UTF_8);
                if (as.creator.size() != 0)
                    creator = new String(as.creator.toByteArray(), StandardCharsets.UTF_8);

                byte[] dataFork = as.data.toByteArray();
                byte[] rsrcFork = as.rsrc.toByteArray();

                HfsFileHandle fh = Hfs.hfsCreate(volume, hfsPath, type, creator);
                try
                {
                    writeFork(fh, FK_DATA, dataFork);
                    writeFork(fh, FK_RSRC, rsrcFork);
                } finally
                {
                    Hfs.hfsClose(fh);
                }
                lastEx = null;
                break;
            } catch (HfsException e)
            {
                lastEx = e;
                if (e.errno == HfsException.EIO && attempt == 0)
                {
                    try
                    {
                        Hfs.hfsFlush(volume);
                    } catch (HfsException flushEx)
                    {
                        // ignore
                    }
                    continue;
                }
                break;
            }
        }
        if (lastEx != null)
            throw mapHfsException(lastEx, path);
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        mount();

        try
        {
            HfsDirEnt ent = new HfsDirEnt();
            Hfs.hfsStat(volume, pathToHfsPath(path), ent);
            return direntFromHfsEnt(path, ent);
        } catch (HfsException e)
        {
            throw mapHfsException(e, path);
        }
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        mount();

        try
        {
            Hfs.hfsMkdir(volume, pathToHfsPath(path));
        } catch (HfsException e)
        {
            throw mapHfsException(e, path);
        }
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        mount();

        try
        {
            String hfsPath = pathToHfsPath(path);
            HfsDirEnt ent = new HfsDirEnt();
            Hfs.hfsStat(volume, hfsPath, ent);

            if ((ent.flags & HFS_ISDIR) != 0)
                Hfs.hfsRmdir(volume, hfsPath);
            else
                Hfs.hfsDelete(volume, hfsPath);
        } catch (HfsException e)
        {
            throw mapHfsException(e, path);
        }
    }

    @Override
    public void moveFile(VfsPath oldName, VfsPath newName) throws IOException
    {
        mount();

        if (oldName.equals(newName))
            throw new InvalidPathException(oldName.toString(), "can't move onto itself");

        // VFS expects FileAlreadyExistsException if destination exists
        try
        {
            getDirent(newName);
            throw new FileAlreadyExistsException(newName.toString());
        } catch (NoSuchFileException e)
        {
            // Destination doesn't exist, proceed
        }

        // For files, do manual copy via AppleSingle to ensure data forks are preserved
        // and to avoid HFS b-tree allocation bugs when moving between directories.
        Dirent oldEnt;
        try
        {
            oldEnt = getDirent(oldName);
        } catch (IOException e)
        {
            oldEnt = null;
        }
        if (oldEnt != null && oldEnt.fileType() == FileType.IS_FILE)
        {
            Bytes data = getFile(oldName);
            deleteFile(oldName);
            try
            {
                flushChanges();
            } catch (IOException e)
            {
                // ignore
            }
            putFile(newName, data);
            return;
        }

        // For directories, use HFS rename (handles valence, thread, etc.)
        // Check for moving directory inside itself
        if (!oldName.isRoot() && newName.toString().startsWith(oldName.toString() + "/"))
        {
            try
            {
                Dirent ent = getDirent(oldName);
                if (ent.fileType() == FileType.IS_DIR)
                    throw new InvalidPathException(
                            newName.toString(),
                            "can't move directory inside itself");
            } catch (NoSuchFileException e)
            {
                // Old doesn't exist, let HFS handle
            }
        }

        try
        {
            String oldPath = pathToHfsPath(oldName);
            String newPath = pathToHfsPath(newName);
            Hfs.hfsRename(volume, oldPath, newPath);
        } catch (HfsException e)
        {
            throw mapHfsException(e, oldName);
        }
    }

    @Override
    public void close() throws Exception
    {
        flushChanges();
    }

    @Override
    public boolean needsFlushing()
    {
        return blockDevice.needsCommit();
    }

    @Override
    public void flushChanges() throws IOException
    {
        unmount();
        blockDevice.commit();
    }

    @Override
    public void discardChanges() throws IOException
    {
        unmount();
        blockDevice.revert();
    }

    private String pathToHfsPath(VfsPath path)
    {
        if (path.segments().isEmpty())
            return ":";
        return ":" + String.join(":", path.segments());
    }

    private Dirent direntFromHfsEnt(VfsPath path, HfsDirEnt ent)
    {
        String name = charsToString(ent.name);
        boolean isDir = (ent.flags & HFS_ISDIR) != 0;
        ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
        Dirent.DirentBuilder b = Dirent.builder().setFilename(name).setPath(path).setMode("");
        attrs.put(Attributes.FILENAME, name);
        if (isDir)
        {
            b.setFileType(FileType.IS_DIR);
            attrs.put(Attributes.FILE_TYPE, "dir");
        } else
        {
            int appleLen = AppleSingle.OVERHEAD + (int) ent.uFile.dsize + (int) ent.uFile.rsize;
            b.setFileType(FileType.IS_FILE).setLength(appleLen);
            attrs.put(Attributes.LENGTH, Integer.toString(appleLen));
            attrs.put(Attributes.FILE_TYPE, "file");
        }
        return b.setAttributes(attrs.build()).build();
    }

    private byte[] readFork(HfsFileHandle fh, int fork, long size) throws HfsException
    {
        if (size == 0)
            return new byte[]{};
        Hfs.hfsSetfork(fh, fork);
        byte[] buf = new byte[(int) size];
        Hfs.hfsRead(fh, buf, size);
        return buf;
    }

    private void writeFork(HfsFileHandle fh, int fork, byte[] data) throws HfsException
    {
        Hfs.hfsSetfork(fh, fork);
        Hfs.hfsWrite(fh, data, data.length);
    }

    private IOException mapHfsException(HfsException e, VfsPath path)
    {
        String msg = e.getMessage() != null ? e.getMessage() : "";
        if (e.errno == HfsException.EINVAL)
            throw new InvalidPathException(path.toString(), msg);
        return switch (e.errno)
        {
            case HfsException.ENOENT, HfsException.ENOTDIR ->
                    new NoSuchFileException(path.toString(), null, msg);
            case HfsException.EEXIST, HfsException.EISDIR ->
                    new FileAlreadyExistsException(path.toString(), null, msg);
            case HfsException.ENOTEMPTY -> new DirectoryNotEmptyException(path.toString());
            default -> new IOException(msg, e);
        };
    }

    private void mount() throws IOException
    {
        if (volume != null)
            return;
        try
        {
            volume = Hfs.hfsMount(hfsos, mountPath, 0, HFS_MODE_RDWR);
        } catch (HfsException e)
        {
            throw mapHfsException(e, VfsPath.of("/"));
        }
    }

    private void unmount() throws IOException
    {
        if (volume != null)
        {
            try
            {
                Hfs.hfsFlush(volume);
                Hfs.hfsUmount(volume);
            } catch (HfsException e)
            {
                throw mapHfsException(e, VfsPath.of("/"));
            }
            volume = null;
        }
    }

    private String charsToString(char[] chars)
    {
        StringBuilder sb = new StringBuilder();
        for (char c : chars)
        {
            if (c == 0)
                break;
            sb.append(c);
        }
        return sb.toString();
    }
}
