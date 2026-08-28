package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_CREATE;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_CREATEDIR;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_DELETE;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_MOVE;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_PUTFILE;
import static com.cowlark.fluxengine.vfs.FileSystem.Capability.OP_PUTFSDATA;
import static com.cowlark.fluxengine.vfs.FileSystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.FileSystem.FileType.IS_FILE;
import static org.elm_chan.ff.DResult.RES_ERROR;
import static org.elm_chan.ff.DResult.RES_OK;
import static org.elm_chan.ff.DResult.RES_PARERR;
import static org.elm_chan.ff.FResult.FR_OK;
import static org.elm_chan.ff.FatFs.FM_ANY;
import static org.elm_chan.ff.FatFs.FM_SFD;
import static org.elm_chan.ff.FilInfo.AM_DIR;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.FileSystem.Dirent.DirentBuilder;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import org.elm_chan.ff.DResult;
import org.elm_chan.ff.Dir;
import org.elm_chan.ff.DiskIo;
import org.elm_chan.ff.FResult;
import org.elm_chan.ff.FatFs;
import org.elm_chan.ff.Fil;
import org.elm_chan.ff.FilInfo;
import org.elm_chan.ff.MkfsParm;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.AccessDeniedException;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.time.LocalDateTime;

public class FatFileSystem extends FileSystem
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
            OP_GETFSDATA,
            OP_PUTFSDATA);

    private final FatFsProto config;
    private final BlockDevice blockDevice;
    private final DiskIoAdapter fatDevice;
    private final FatFs fatFilesystem;

    private static class DiskIoAdapter implements DiskIo
    {
        private final BlockDevice underlying;

        DiskIoAdapter(BlockDevice underlying)
        {
            this.underlying = underlying;
        }

        @Override
        public int diskInitialize()
        {
            return 0;
        }

        @Override
        public int diskStatus()
        {
            return 0;
        }

        @Override
        public DResult diskRead(long sector, byte[] buff, int count)
        {
            try
            {
                Bytes bytes = underlying.getBlocks((int) sector, count);
                byte[] src = bytes.toByteArray();
                System.arraycopy(src, 0, buff, 0, src.length);
                return RES_OK;
            } catch (IOException e)
            {
                return RES_ERROR;
            }
        }

        @Override
        public DResult diskWrite(long sector, byte[] buff, int count)
        {
            try
            {
                underlying.putBlocks((int) sector, new Bytes(buff));
                return RES_OK;
            } catch (IOException e)
            {
                return RES_ERROR;
            }
        }

        @Override
        public DResult diskRead(long sector, ByteBuffer buff, int count)
        {
            try
            {
                Bytes bytes = underlying.getBlocks((int) sector, count);
                byte[] src = bytes.toByteArray();
                // Use absolute put to avoid touching position/limit
                for (int i = 0; i < src.length; i++)
                    buff.put(i, src[i]);
                return RES_OK;
            } catch (IOException e)
            {
                return RES_ERROR;
            }
        }

        @Override
        public DResult diskWrite(long sector, ByteBuffer buff, int count)
        {
            try
            {
                byte[] tmp = new byte[count * 512];
                // Use absolute get to avoid touching position/limit
                for (int i = 0; i < tmp.length; i++)
                    tmp[i] = buff.get(i);
                underlying.putBlocks((int) sector, new Bytes(tmp));
                return RES_OK;
            } catch (IOException e)
            {
                return RES_ERROR;
            }
        }

        @Override
        public DResult diskIoctl(int cmd, Object buff)
        {
            switch (cmd)
            {
                case GET_SECTOR_COUNT:
                    if (buff instanceof long[] ibuff)
                    {
                        ibuff[0] = underlying.getBlockCount();
                        return RES_OK;
                    }
                    if (buff instanceof int[] ibuff2)
                    {
                        ibuff2[0] = underlying.getBlockCount();
                        return RES_OK;
                    }
                    break;

                case GET_SECTOR_SIZE:
                    if (buff instanceof long[] ibuff)
                    {
                        ibuff[0] = underlying.getBlockSize();
                        return RES_OK;
                    }
                    if (buff instanceof int[] ibuff2)
                    {
                        ibuff2[0] = underlying.getBlockSize();
                        return RES_OK;
                    }
                    break;

                case GET_BLOCK_SIZE:
                    if (buff instanceof long[] ibuff)
                    {
                        ibuff[0] = 1;
                        return RES_OK;
                    }
                    if (buff instanceof int[] ibuff2)
                    {
                        ibuff2[0] = 1;
                        return RES_OK;
                    }
                    break;

                case CTRL_SYNC:
                    return RES_OK;
            }

            return RES_PARERR;
        }
    }

    public FatFileSystem(FatFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        this.fatDevice = new DiskIoAdapter(blockDevice);
        this.fatFilesystem = new FatFs(fatDevice, FatFileSystem::getTime);
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        MkfsParm params = new MkfsParm();
        params.fmt = FM_SFD | FM_ANY;
        params.nFat = 1;
        int nRoot = 0;
        long auSize = 0;
        if (config != null)
        {
            if (config.hasRootDirectoryEntries())
                nRoot = config.getRootDirectoryEntries();
            if (config.hasClusterSize())
                auSize = config.getClusterSize();
        }
        params.nRoot = nRoot;
        params.auSize = auSize;
        checkResult(fatFilesystem.mkfs("", params));
        checkResult(fatFilesystem.setLabel(volumeName));
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        mount();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();

        String[] stringBox = new String[1];
        long[] longBox = new long[1];
        checkResult(fatFilesystem.getLabel("", stringBox, longBox));
        builder.put(Attributes.VOLUME_NAME, stringBox[0]);

        checkResult(fatFilesystem.getfree("", longBox));
        long total = (fatFilesystem.n_fatent - 2) + (fatFilesystem.database / fatFilesystem.csize);
        builder.put(Attributes.TOTAL_BLOCKS, Long.toString(total));
        builder.put(Attributes.USED_BLOCKS, Long.toString(total - longBox[0]));
        builder.put(
                Attributes.BLOCK_SIZE,
                Long.toString(fatFilesystem.csize * blockDevice.getBlockSize()));

        return builder.build();
    }

    @Override
    public void putFilesystemMetadata(ImmutableMap<String, String> metadata) throws IOException
    {
        mount();

        if (!metadata.keySet().equals(ImmutableSet.of(Attributes.VOLUME_NAME)))
            throw new IllegalArgumentException("can't set this metadata key");

        checkResult(fatFilesystem.setLabel(metadata.get(Attributes.VOLUME_NAME)));
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        mount();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();

        String p = toFatPath(path);
        Dir dir = new Dir();
        checkResult(fatFilesystem.opendir(dir, p));

        FilInfo filinfo = new FilInfo();
        for (; ; )
        {
            checkResult(fatFilesystem.readdir(dir, filinfo));
            if (filinfo.fname.isEmpty())
                break;

            builder.put(filinfo.fname, makeDirent(path, filinfo));
        }

        return builder.build();
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        mount();
        Fil fil = new Fil();

        checkResult(fatFilesystem.open(fil, toFatPath(path), FatFs.FA_READ));
        try
        {
            int size = (int) fil.objsize;
            ByteBuffer buf = ByteBuffer.allocate(size);

            int[] br = new int[1];
            checkResult(fatFilesystem.read(fil, buf, size, br));
            return new Bytes(buf);
        } finally
        {
            fatFilesystem.close(fil);
        }
    }

    @Override
    public void createDirectory(VfsPath path) throws IOException
    {
        mount();
        checkResult(fatFilesystem.mkdir(toFatPath(path)));
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        mount();
        String fatPath = toFatPath(path);

        FilInfo filinfo = new FilInfo();
        FResult res = fatFilesystem.stat(fatPath, filinfo);
        if ((res == FR_OK) && ((filinfo.fattrib & AM_DIR) != 0))
            throw new FileAlreadyExistsException("file already exists");

        Fil fil = new Fil();
        checkResult(fatFilesystem.open(fil, fatPath, FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS));
        try
        {
            ByteBuffer buf = bytes.toByteBuffer();

            int[] bw = new int[1];
            checkResult(fatFilesystem.write(fil, buf, bytes.size(), bw));
        } finally
        {
            checkResult(fatFilesystem.close(fil));
        }
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        mount();
        String p = toFatPath(path);
        FilInfo filinfo = new FilInfo();
        checkResult(fatFilesystem.stat(p, filinfo));
        VfsPath parent = path.getParent();
        if (parent == null)
            parent = VfsPath.of("/");
        return makeDirent(parent, filinfo);
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        mount();
        String p = toFatPath(path);
        FResult res = fatFilesystem.unlink(p);
        if (res == FResult.FR_DENIED)
            throw new DirectoryNotEmptyException(p);
        checkResult(res);
    }

    @Override
    public void moveFile(VfsPath oldName, VfsPath newName) throws IOException
    {
        mount();
        String oldP = toFatPath(oldName);
        String newP = toFatPath(newName);
        checkResult(fatFilesystem.rename(oldP, newP));
    }

    private static Dirent makeDirent(VfsPath dir, FilInfo fi)
    {
        ImmutableMap.Builder<String, String> attrsBuilder = ImmutableMap.builder();
        DirentBuilder direntBuilder =
                Dirent.builder().setFilename(fi.fname).setPath(dir.resolve(fi.fname)).setMode("");

        attrsBuilder.put(Attributes.FILENAME, fi.fname);

        if ((fi.fattrib & AM_DIR) != AM_DIR)
        {
            long length = fi.fsize;
            direntBuilder.setFileType(IS_FILE).setLength((int) length);
            attrsBuilder
                    .put(Attributes.LENGTH, Long.toString(length))
                    .put(Attributes.FILE_TYPE, "file");
        } else
        {
            direntBuilder.setFileType(IS_DIR);
            attrsBuilder.put(Attributes.FILE_TYPE, "dir");
        }

        return direntBuilder.setAttributes(attrsBuilder.build()).build();
    }

    private void mount() throws IOException
    {
        FResult res = fatFilesystem.mount();
        checkResult(res);
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
    }

    @Override
    public void discardChanges()
    {
        fatFilesystem.unmount("");
        blockDevice.revert();
    }

    private static String toFatPath(VfsPath path)
    {
        if (path == null)
            return "/";
        return path.toString();
    }

    private static void checkResult(FResult result) throws IOException
    {
        switch (result)
        {
            case FR_OK:
                return;
            case FR_NO_FILE:
            case FR_NO_PATH:
                throw new NoSuchFileException("no such file: " + result);

            case FR_INVALID_NAME:
                throw new InvalidPathException("", "invalid path: " + result);

            case FR_EXIST:
                throw new FileAlreadyExistsException("file already exists: " + result);

            case FR_DENIED:
                throw new AccessDeniedException("access denied");

            default:
                throw new IOException(String.format("filesystem error: %s", result));
        }
    }

    private static long getTime()
    {
        LocalDateTime now = LocalDateTime.now();
        int year = now.getYear() - 1980;
        if (year < 0)
            year = 0;
        if (year > 127)
            year = 127;
        int mon = now.getMonthValue();
        int day = now.getDayOfMonth();
        int hour = now.getHour();
        int min = now.getMinute();
        int sec = now.getSecond();
        int date = (year << 9) | (mon << 5) | day;
        int time = (hour << 11) | (min << 5) | (sec / 2);
        return ((long) date << 16) | (time & 0xFFFF);
    }
}
