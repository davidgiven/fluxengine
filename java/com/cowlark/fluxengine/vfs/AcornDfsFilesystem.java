package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.ByteWriter;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import org.apache.commons.lang3.StringUtils;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class AcornDfsFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final AcornDfsProto config;
    private final BlockDevice blockDevice;

    private static class DirEntry
    {
        String filename;
        int inode;
        int startSector;
        int loadAddress;
        int execAddress;
        int length;
        int sectorCount;
        boolean locked;
        String mode;
        Dirent dirent;
    }

    private class AcornDfsDirectory
    {
        String volumeName;
        int usedSectors = 0;
        List<DirEntry> dirents = new ArrayList<>();

        AcornDfsDirectory() throws IOException
        {
            Bytes sector0 = blockDevice.getBlock(0);
            Bytes sector1 = blockDevice.getBlock(1);

            if ((sector1.getByte(5) & 7) != 0)
                throw new FileSystemException("Invalid filesystem");

            int fileCount = (sector1.getByte(5) & 0xff) / 8;

            for (int i = 0; i < fileCount; i++)
            {
                Bytes bytes0 = sector0.slice(i * 8 + 8, 8);
                Bytes bytes1 = sector1.slice(i * 8 + 8, 8);
                DirEntry de = createDirent(i, bytes0, bytes1);
                usedSectors += de.sectorCount;
                dirents.add(de);
            }

            StringBuilder ss = new StringBuilder();
            for (int i = 0; i < 8; i++)
                ss.append((char) (sector0.getByte(i) & 0x7f));
            for (int i = 0; i < 4; i++)
                ss.append((char) (sector1.getByte(i) & 0x7f));
            volumeName = StringUtils.stripEnd(ss.toString(), null);
        }

        DirEntry findFile(VfsPath path) throws IOException
        {
            if (path.segments().size() != 1)
                throw new InvalidPathException(path.toString(), "Bad path");

            String wanted = path.segments().get(0);
            for (DirEntry dirent : dirents)
            {
                if (dirent.filename.equals(wanted))
                    return dirent;
            }

            throw new NoSuchFileException(path.toString());
        }

        private DirEntry createDirent(int inode, Bytes bytes0, Bytes bytes1)
        {
            StringBuilder sb = new StringBuilder();
            sb.append((char) (bytes0.getByte(7) & 0x7f));
            sb.append('.');
            for (int j = 0; j < 7; j++)
                sb.append((char) (bytes0.getByte(j) & 0x7f));
            String filename = sb.toString();
            int spaceIdx = filename.indexOf(' ');
            if (spaceIdx != -1)
                filename = filename.substring(0, spaceIdx);

            int startSector = ((bytes1.getByte(6) & 0x03) << 8) | (bytes1.getByte(7) & 0xff);
            int loadAddress = ((bytes1.getByte(6) & 0x0c) << 14) | (bytes1.getByte(1) << 8) |
                    bytes1.getByte(0);
            int execAddress = ((bytes1.getByte(6) & 0xc0) << 10) | (bytes1.getByte(3) << 8) |
                    bytes1.getByte(2);
            boolean locked = (bytes0.getByte(7) & 0x80) != 0;
            int length = ((bytes1.getByte(6) & 0x30) << 12) | (bytes1.getByte(5) << 8) |
                    bytes1.getByte(4);
            int sectorCount = (length + 255) / 256;
            String mode = locked ? "L" : "";

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put("acorndfs.inode", Integer.toString(inode));
            attrs.put("acorndfs.start_sector", Integer.toString(startSector));
            attrs.put("acorndfs.load_address", String.format("0x%x", loadAddress));
            attrs.put("acorndfs.exec_address", String.format("0x%x", execAddress));
            attrs.put("acorndfs.locked", locked ? "1" : "0");

            Dirent dirent = Dirent
                    .builder()
                    .setPath(VfsPath.of("/").resolve(filename))
                    .setFilename(filename)
                    .setLength(length)
                    .setMode(mode)
                    .setFileType(IS_FILE)
                    .setAttributes(attrs.build())
                    .build();

            DirEntry de = new DirEntry();
            de.filename = filename;
            de.inode = inode;
            de.startSector = startSector;
            de.loadAddress = loadAddress;
            de.execAddress = execAddress;
            de.length = length;
            de.sectorCount = sectorCount;
            de.locked = locked;
            de.mode = mode;
            de.dirent = dirent;
            return de;
        }
    }

    public AcornDfsFilesystem(AcornDfsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        AcornDfsDirectory dir = new AcornDfsDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, dir.volumeName);
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(blockDevice.getBlockCount()));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedSectors));
        builder.put(Attributes.BLOCK_SIZE, "256");
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        AcornDfsDirectory dir = new AcornDfsDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (DirEntry de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        AcornDfsDirectory dir = new AcornDfsDirectory();
        DirEntry de = dir.findFile(path);

        int sectors = (de.length + 255) / 256;
        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (int i = 0; i < sectors; i++)
        {
            Bytes sector = blockDevice.getBlock(de.startSector + i);
            bw.write(sector);
        }
        if (data.size() > de.length)
            data.resize(de.length);
        return data;
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        AcornDfsDirectory dir = new AcornDfsDirectory();
        return dir.findFile(path).dirent;
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
    public void discardChanges() throws IOException
    {
        blockDevice.revert();
    }
}
