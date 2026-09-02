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
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.Bytes;
import com.github.adflib.adflib.AdfBitm;
import com.github.adflib.adflib.AdfConstants;
import com.github.adflib.adflib.AdfDir;
import com.github.adflib.adflib.AdfDisk;
import com.github.adflib.adflib.AdfError;
import com.github.adflib.adflib.AdfFile;
import com.github.adflib.adflib.AdfHd;
import com.github.adflib.adflib.AdfList;
import com.github.adflib.adflib.AdfRaw;
import com.github.adflib.adflib.BRootBlock;
import com.github.adflib.adflib.Device;
import com.github.adflib.adflib.Entry;
import com.github.adflib.adflib.File;
import com.github.adflib.adflib.Volume;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.Arrays;

public class AmigaFilesystem extends Filesystem
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

    private final AmigaFfsProto config;
    private final BlockDevice blockDevice;
    private final Device amigaDevice;
    private Volume volume;

    private class AmigaDevice extends Device
    {
        private final BlockDevice underlying;

        AmigaDevice(BlockDevice underlying)
        {
            this.underlying = underlying;
            this.size = underlying.getBlockCount() * underlying.getBlockSize();
            this.readOnly = false;
            this.cylinders = 80;
            this.heads = 2;
            this.sectors = 11;
            if (size == 512 * 22 * 2 * 80)
                this.sectors = 22;
            this.devType = AdfHd.adfDevType(this);
            if (this.devType == -1)
                this.devType = AdfConstants.DEVTYPE_FLOPDD;
        }

        @Override
        public AdfError adfReadSector(int n, int size, ByteBuffer buf)
        {
            try
            {
                int blockSize = underlying.getBlockSize();
                /* Handle 256-byte RDSK etc. via read-modify */
                if (size == 512)
                {
                    Bytes bytes = underlying.getBlocks(n, 1);
                    byte[] src = bytes.toByteArray();
                    for (int i = 0; i < src.length && i < size; i++)
                        buf.put(i, src[i]);
                    return AdfError.RC_OK;
                } else
                {
                    /* Sub-sector (e.g., 256 for RDSK): read full block and slice */
                    int blockNum = n * size / blockSize;
                    int offsetInBlock = (n * size) % blockSize;
                    Bytes bytes = underlying.getBlocks(blockNum, 1);
                    byte[] src = bytes.toByteArray();
                    for (int i = 0; i < size; i++)
                        buf.put(i, src[offsetInBlock + i]);
                    return AdfError.RC_OK;
                }
            } catch (IOException e)
            {
                return AdfError.RC_ERROR;
            }
        }

        @Override
        public AdfError adfReadSector(int n, int size, byte[] buf)
        {
            return adfReadSector(n, size, ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN));
        }

        @Override
        public AdfError adfWriteSector(int n, int size, ByteBuffer buf)
        {
            try
            {
                int blockSize = underlying.getBlockSize();
                if (size == 512 && size == blockSize)
                {
                    byte[] tmp = new byte[size];
                    for (int i = 0; i < size; i++)
                        tmp[i] = buf.get(i);
                    underlying.putBlocks(n, new Bytes(tmp));
                    return AdfError.RC_OK;
                } else
                {
                    /* Sub-sector write: read-modify-write full block */
                    int blockNum = n * size / blockSize;
                    int offsetInBlock = (n * size) % blockSize;
                    Bytes existing = underlying.getBlocks(blockNum, 1);
                    byte[] blockData = existing.toByteArray();
                    for (int i = 0; i < size; i++)
                        blockData[offsetInBlock + i] = buf.get(i);
                    underlying.putBlocks(blockNum, new Bytes(blockData));
                    return AdfError.RC_OK;
                }
            } catch (IOException e)
            {
                return AdfError.RC_ERROR;
            }
        }

        @Override
        public AdfError adfWriteSector(int n, int size, byte[] buf)
        {
            return adfWriteSector(n, size, ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN));
        }

        @Override
        public AdfError adfReleaseDevice()
        {
            return AdfError.RC_OK;
        }
    }

    public AmigaFilesystem(AmigaFfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        this.amigaDevice = new AmigaDevice(blockDevice);
    }

    /* Visible for testing */
    Device getAmigaDevice()
    {
        return amigaDevice;
    }

    Volume getVolume()
    {
        return volume;
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        /* Use adfCreateFlop for floppy, adfCreateVol for more control */
        String volName = volumeName != null ? volumeName : "empty";
        /* Truncate to 30 chars (Amiga vol name limit) and uppercase */
        if (volName.length() > 30)
            volName = volName.substring(0, 30);

        /* Match the old C++ amigaffs.cc which passes 0 (FS_OFS).
         * Enabling DIRCACHE (via FSMASK_DIRCACHE) triggers buggy dir-cache
         * paths in the adflib translation (AIOOBE in adfSetBlockFree,
         * corrupted dir-cache blocks causing mount failures, etc.). */
        int volType = AdfConstants.FS_OFS;
        AdfError rc;
        if (amigaDevice.devType == AdfConstants.DEVTYPE_HARDDISK)
            /* For hard disk, need partition handling - simplify to single partition FFS */
            rc = AdfHd.adfCreateFlop(amigaDevice, volName, volType);
        else
            rc = AdfHd.adfCreateFlop(amigaDevice, volName, volType);
        checkResult(rc);
        mount();
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        mount();
        String volName = volume.volName != null ? volume.volName : "";
        int freeBlocks = AdfBitm.adfCountFreeBlocks(volume);
        int totalBlocks = volume.lastBlock - volume.firstBlock + 1;
        /* Approximation: total blocks from device */
        if (totalBlocks <= 0)
            totalBlocks = amigaDevice.size / 512;
        return ImmutableMap
                .<String, String>builder()
                .put(Attributes.VOLUME_NAME, volName)
                .put(Attributes.TOTAL_BLOCKS, Integer.toString(totalBlocks))
                .put(Attributes.USED_BLOCKS, Integer.toString(totalBlocks - freeBlocks))
                .put(Attributes.BLOCK_SIZE, Integer.toString(volume.blockSize))
                .build();
    }

    @Override
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata) throws IOException
    {
        mount();

        if (!metadata.keySet().equals(ImmutableSet.of(Attributes.VOLUME_NAME)))
            throw new IllegalArgumentException("can't set this metadata key");

        /* Read the root block, modify the name, write it back.
         * adfWriteRootBlock recomputes the checksum internally. */
        BRootBlock root = new BRootBlock();
        checkResult(AdfRaw.adfReadRootBlock(volume, volume.rootBlock, root));

        String newName = Strings.nullToEmpty(metadata.get(Attributes.VOLUME_NAME));
        int nlen = Math.min(AdfConstants.MAXNAMELEN, newName.length());
        root.nameLen = (byte) nlen;
        byte[] nameBytes = newName.getBytes();
        Arrays.fill(root.diskName, (byte) 0);
        for (int i = 0; i < nlen; i++)
            root.diskName[i] = nameBytes[i];

        volume.volName = newName.substring(0, nlen);

        checkResult(AdfRaw.adfWriteRootBlock(volume, volume.rootBlock, root));
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        mount();
        int parentBlock = resolveDir(path);

        AdfList list = AdfDir.adfGetDirEnt(volume, parentBlock);
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (AdfList node = list; node != null; node = node.next)
        {
            Entry e = (Entry) node.content;
            if (e.name.equals(".") || e.name.equals(".."))
                continue;
            builder.put(e.name, makeDirent(path, e));
        }
        AdfDir.adfFreeDirList(list);
        return builder.build();
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        mount();
        Entry entry = findEntry(path);
        if (entry.type != AdfConstants.ST_FILE)
            throw new NoSuchFileException("not a file: " + path);
        int parentBlock = getParentBlock(path);
        String baseName = path.getName();
        volume.curDirPtr = parentBlock;

        File file = AdfFile.adfOpenFile(volume, baseName, "r");
        AdfDir.adfToRootDir(volume);
        if (file == null)
            throw new NoSuchFileException(path.toString());
        try
        {
            int size = (int) entry.size;
            ByteBuffer buf = ByteBuffer.allocate(size).order(ByteOrder.BIG_ENDIAN);
            int read = AdfFile.adfReadFile(file, size, buf);
            if (read < 0)
                throw new IOException("read failed");
            byte[] out = new byte[size];
            for (int i = 0; i < size; i++)
                out[i] = buf.get(i);
            return new Bytes(out);
        } finally
        {
            AdfFile.adfCloseFile(file);
        }
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        mount();

        /* If it's a directory, fail like FatFileSystem does */
        Entry existing = findEntryOrNull(path);
        if (existing != null && existing.type == AdfConstants.ST_DIR)
            throw new FileAlreadyExistsException(path.toString());

        /* Remove existing file if present (like FatFileSystem's CREATE_ALWAYS) */
        if (existing != null)
        {
            int parent = getParentBlock(path);
            String name = path.getName();
            checkResult(AdfDir.adfRemoveEntry(volume, parent, name));
        }
        int parentBlock = getParentBlock(path);
        String baseName = path.getName();
        volume.curDirPtr = parentBlock;
        File file = AdfFile.adfOpenFile(volume, baseName, "w");
        AdfDir.adfToRootDir(volume);
        if (file == null)
            throw new IOException("cannot create file: " + path);
        try
        {
            ByteBuffer buf = bytes.toByteBuffer();
            /* Ensure we copy without touching position */
            ByteBuffer dup = buf.duplicate();
            int written = AdfFile.adfWriteFile(file, bytes.size(), dup);
            if (written != bytes.size())
                throw new IOException("short write");
        } finally
        {
            AdfFile.adfCloseFile(file);
        }
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        mount();
        Entry e = findEntry(path);
        VfsPath parent = path.getParent();
        if (parent == null)
            parent = VfsPath.of("/");
        return makeDirent(parent, e);
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        mount();
        VfsPath parentPath = path.getParent();
        if (parentPath == null)
            parentPath = VfsPath.of("/");
        String name = path.getName();
        int parentBlock = resolveDir(parentPath);

        /* Check if name already exists in parent directory before creating.
         * adfCreateDir returns RC_ERROR for both "name exists" and "no sector",
         * so we need an explicit check to throw FileAlreadyExistsException. */
        int savedDirPtr = volume.curDirPtr;
        volume.curDirPtr = parentBlock;
        Entry existing = AdfDir.adfFindEntry(volume, name);
        volume.curDirPtr = savedDirPtr;
        if (existing != null)
            throw new FileAlreadyExistsException(path.toString());

        checkResult(AdfDir.adfCreateDir(volume, parentBlock, name));
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        mount();
        Entry e = findEntry(path);
        if (e.type == AdfConstants.ST_DIR)
        {
            /* Check if directory is empty */
            AdfList list = AdfDir.adfGetDirEnt(volume, e.sector);
            boolean empty = true;
            for (AdfList n = list; n != null; n = n.next)
            {
                Entry child = (Entry) n.content;
                if (!child.name.equals(".") && !child.name.equals(".."))
                {
                    empty = false;
                    break;
                }
            }
            AdfDir.adfFreeDirList(list);
            if (!empty)
                throw new DirectoryNotEmptyException(path.toString());
        }
        int parent = getParentBlock(path);
        String name = path.getName();
        checkResult(AdfDir.adfRemoveEntry(volume, parent, name));
    }

    @Override
    public void moveFile(VfsPath oldName, VfsPath newName) throws IOException
    {
        mount();

        /* Check for moving directory into itself — must come before the
         * existing-check so that moving /dir onto /dir itself gives
         * InvalidPathException rather than FileAlreadyExistsException. */
        Entry oldEntry = findEntry(oldName);
        if (oldEntry.type == AdfConstants.ST_DIR)
            if (newName.equals(oldName) || newName.toString().startsWith(oldName.toString() + "/"))
                throw new InvalidPathException(
                        newName.toString(),
                        "cannot move directory into itself");

        /* Check if target already exists */
        Entry existing = findEntryOrNull(newName);
        if (existing != null)
            throw new FileAlreadyExistsException(newName.toString());

        int oldParent = getParentBlock(oldName);
        String oldBase = oldName.getName();
        int newParent = getParentBlock(newName);
        String newBase = newName.getName();

        checkResult(AdfDir.adfRenameEntry(volume, oldParent, oldBase, newParent, newBase));
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
        blockDevice.commit();

        /* Unmount the volume after flushing so adflib's internal state
         * (bitmap cache, etc.) is reset. The next operation will
         * re-mount via mount() which calls adfMountDev + adfMount. */
        if (volume != null)
        {
            AdfDisk.adfUnMount(volume);
            volume = null;
        }
    }

    @Override
    public void discardChanges()
    {
        if (volume != null)
        {
            AdfDisk.adfUnMount(volume);
            volume = null;
        }
        blockDevice.revert();
    }

    private static Dirent makeDirent(VfsPath dir, Entry e)
    {
        ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
        Dirent.DirentBuilder b =
                Dirent.builder().setFilename(e.name).setPath(dir.resolve(e.name)).setMode("");
        attrs.put(Attributes.FILENAME, e.name);
        if (e.type == AdfConstants.ST_FILE)
        {
            b.setFileType(IS_FILE).setLength((int) e.size);
            attrs.put(Attributes.LENGTH, Long.toString(e.size)).put(Attributes.FILE_TYPE, "file");
        } else
        {
            b.setFileType(IS_DIR);
            attrs.put(Attributes.FILE_TYPE, "dir");
        }
        return b.setAttributes(attrs.build()).build();
    }

    private void mount() throws IOException
    {
        if (volume != null && volume.mounted)
            return;

        /* Populate volList/nVol/geometry from current disk state.
         * On a fresh instance (without create()), adfCreateFlop was never called,
         * so volList is empty and AdfDisk.adfMount would return null. */

        AdfHd.adfMountDev(amigaDevice);
        volume = AdfDisk.adfMount(amigaDevice, 0, false);
        if (volume == null)
            throw new IOException("mount failed");
    }

    private Entry findEntry(VfsPath path) throws IOException
    {
        Entry e = findEntryOrNull(path);
        if (e == null)
            throw new NoSuchFileException(path.toString());
        return e;
    }

    private Entry findEntryOrNull(VfsPath path)
    {
        if (path.segments().isEmpty())
            return null; /* root has no Entry, but treat as found for dir ops */

        /* Use adfFindEntry which searches from current dir? We need to handle
         * absolute paths.  adfFindEntry expects name relative to vol.curDirPtr,
         * but we can use absolute by resetting to root. */
        AdfDir.adfToRootDir(volume);

        /* Walk the path segments */
        int n = path.segments().size();
        Entry cur = null;
        for (int i = 0; i < n; i++)
        {
            String part = path.segments().get(i);
            cur = AdfDir.adfFindEntry(volume, part);
            if (cur == null)
            {
                AdfDir.adfToRootDir(volume);
                return null;
            }

            /* If not last part, need to change dir to this entry's sector for
             * next lookup.  For simplicity, use adfChangeDir to descend if entry
             * is dir and not last.  But adfFindEntry already searches current
             * dir, so we need to update curDirPtr.  Instead, just keep cur and
             * for next iteration, change dir. */
            if (i != n - 1 && cur.type == AdfConstants.ST_DIR)
                volume.curDirPtr = cur.sector;
        }

        /* Restore to root after search */
        AdfDir.adfToRootDir(volume);
        return cur;
    }

    private int resolveDir(VfsPath path) throws IOException
    {
        AdfDir.adfToRootDir(volume);
        if (path.segments().isEmpty())
            return volume.curDirPtr;

        int cur = volume.curDirPtr;
        for (String part : path.segments())
        {
            Entry e = AdfDir.adfFindEntry(volume, part);
            if (e == null)
                throw new NoSuchFileException(path.toString());
            if (e.type != AdfConstants.ST_DIR)
                throw new NoSuchFileException("not a directory: " + part);
            cur = e.sector;
            volume.curDirPtr = cur;
        }

        int res = cur;
        AdfDir.adfToRootDir(volume);
        return res;
    }

    private int getParentBlock(VfsPath path) throws IOException
    {
        VfsPath parent = path.getParent();
        if (parent == null)
            parent = VfsPath.of("/");
        return resolveDir(parent);
    }

    private static void checkResult(AdfError rc) throws IOException
    {
        if (rc == AdfError.RC_OK)
            return;
        if (rc == AdfError.RC_ERROR)
            throw new IOException("filesystem error: " + rc);
        throw new IOException("filesystem error: " + rc);
    }
}
