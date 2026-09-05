package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class RolandFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final RolandFsProto config;
    private final BlockDevice blockDevice;

    public RolandFilesystem(RolandFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private static String unmangleFilename(String mangled)
    {
        String extension = mangled.substring(10);
        int end = extension.length();
        while (end > 0 && extension.charAt(end - 1) == '_')
            end--;
        extension = extension.substring(0, end);

        String root = mangled.substring(0, 10);
        end = root.length();
        while (end > 0 && root.charAt(end - 1) == '_')
            end--;
        root = root.substring(0, end);

        if (!extension.isEmpty())
            return root + "." + extension;
        return root;
    }

    private int getBlockSize()
    {
        if (config != null)
            return config.getBlockSize();
        return 3072;
    }

    private int getDirectoryTrack()
    {
        if (config != null)
            return config.getDirectoryTrack();
        return 39;
    }

    private int getDirectoryEntries()
    {
        if (config != null)
            return config.getDirectoryEntries();
        return 79;
    }

    private int getLogicalSectorSize()
    {
        // corresponds to getLogicalSectorSize(0,0) in C++
        CylinderHead ch = new CylinderHead(0, 0);
        var ltl = blockDevice.diskLayout.layoutByLogicalLocation.get(ch);
        if (ltl != null)
            return ltl.sectorSize;
        // fallback to blockDevice block size if layout not found
        return blockDevice.getBlockSize();
    }

    private int getLogicalSectorCount()
    {
        return blockDevice.getBlockCount();
    }

    private int getOffsetOfSector(int track, int side, int sector) throws IOException
    {
        CylinderHeadSector loc = new CylinderHeadSector(track, side, sector);
        Long offset = blockDevice.diskLayout.sectorOffsetByLogicalSectorLocation.get(loc);
        if (offset == null)
            throw new FileSystemException("sector not found: " + loc);
        return offset.intValue();
    }

    private Geometry computeGeometry() throws IOException
    {
        Geometry g = new Geometry();
        g.blockSize = getBlockSize();
        g.directoryTrack = getDirectoryTrack();
        g.directoryEntries = getDirectoryEntries();
        g.sectorSize = getLogicalSectorSize();
        if (g.sectorSize == 0)
            g.sectorSize = 256;
        g.blockSectors = g.blockSize / g.sectorSize;
        if (g.blockSectors <= 0)
            g.blockSectors = 1;
        int totalSectors = getLogicalSectorCount();
        g.filesystemBlocks = totalSectors / g.blockSectors;
        try
        {
            g.directoryByteOffset = getOffsetOfSector(g.directoryTrack, 0, 0);
        } catch (IOException e)
        {
            // fallback to linear calculation if layout lookup fails
            g.directoryByteOffset = g.directoryTrack * g.blockSectors * g.sectorSize;
        }
        g.directorySector = g.directoryByteOffset / g.sectorSize;
        g.midBlock = (totalSectors - g.directorySector) / g.blockSectors;
        return g;
    }

    private int blockToLogicalSectorNumber(int block, Geometry g)
    {
        int track;
        if (block < g.midBlock)
            track = g.directoryTrack + block;
        else
            track = g.directoryTrack - (1 + block - g.midBlock);
        return track * g.blockSectors;
    }

    private Bytes getRolandBlock(int number, Geometry g) throws IOException
    {
        int lba = blockToLogicalSectorNumber(number, g);
        return blockDevice.getBlocks(lba, g.blockSectors);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        RolandDirectory dir = new RolandDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(dir.totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(dir.geometry.blockSize));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        RolandDirectory dir = new RolandDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (RolandEntry de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        RolandDirectory dir = new RolandDirectory();
        String wanted = path.segments().get(0);
        return dir.findFile(wanted).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        RolandDirectory dir = new RolandDirectory();
        String wanted = path.segments().get(0);
        RolandEntry de = dir.findFile(wanted);

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (int blockNum : de.blocks)
        {
            Bytes blockData = getRolandBlock(blockNum, dir.geometry);
            bw.write(blockData);
        }
        // truncate to actual file length (blocks * blockSize, but for safety)
        // In C++ they return full concatenated blocks; we keep same without extra truncate.
        // However task says truncate, so ensure we don't exceed expected length.
        int expectedLength = de.blocks.size() * dir.geometry.blockSize;
        if (data.size() > expectedLength)
            data.resize(expectedLength);
        return data;
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

    private static class Geometry
    {
        int blockSize;
        int directoryTrack;
        int directoryEntries;
        int sectorSize;
        int blockSectors;
        int filesystemBlocks;
        int midBlock;
        int directoryByteOffset;
        int directorySector;
    }

    private static class RolandEntry
    {
        String filename;
        List<Integer> blocks = new ArrayList<>();
        Dirent dirent;

        RolandEntry(String filename)
        {
            this.filename = filename;
        }
    }

    private class RolandDirectory
    {
        Geometry geometry;
        List<RolandEntry> dirents = new ArrayList<>();
        Map<String, RolandEntry> map = new LinkedHashMap<>();
        int usedBlocks;
        int totalBlocks;

        RolandDirectory() throws IOException
        {
            geometry = computeGeometry();
            Bytes directory = getRolandBlock(0, geometry);
            if (directory.size() < 32)
                throw new FileSystemException("Invalid filesystem");

            ByteReader hdrReader = new ByteReader(directory);
            hdrReader.seek(1);
            if (hdrReader.remaining() < 13)
                throw new FileSystemException("Invalid filesystem");
            Bytes hdrBytes = hdrReader.read(13);
            String hdr = new String(hdrBytes.toByteArray(), StandardCharsets.ISO_8859_1);
            if (!"ROLAND-GCRDOS".equals(hdr))
                throw new FileSystemException("Invalid filesystem");

            ByteReader br = new ByteReader(directory);
            br.seek(32);

            for (int i = 0; i < geometry.directoryEntries; i++)
            {
                if (br.remaining() < 32)
                    break;
                Bytes direntBytes = br.read(32);
                if ((direntBytes.getByte(0) & 0xff) != 0)
                    continue;

                int extent = direntBytes.getByte(15) & 0xff;
                Bytes mangledBytes = direntBytes.slice(1, 13);
                String mangled =
                        new String(mangledBytes.toByteArray(), StandardCharsets.ISO_8859_1);
                String filename = unmangleFilename(mangled);
                if (filename.isEmpty())
                    continue;

                RolandEntry de = map.get(filename);
                if (de == null)
                {
                    de = new RolandEntry(filename);
                    map.put(filename, de);
                    dirents.add(de);
                }

                for (int j = 0; j < 16; j++)
                {
                    int blockNumber = direntBytes.getByte(16 + j) & 0xff;
                    if (blockNumber == 0)
                        break;
                    int offset = extent * 16 + j;
                    while (de.blocks.size() <= offset)
                        de.blocks.add(0);
                    de.blocks.set(offset, blockNumber);
                }
            }

            // Trim trailing zeros that may have been left as placeholders?
            // In C++ blocks size is offset+1; no gaps, so we keep as is.
            // But ensure we remove trailing zeros if any
            for (RolandEntry de : dirents)
            {
                // Remove trailing zero padding if any (should not happen)
                while (!de.blocks.isEmpty() && de.blocks.get(de.blocks.size() - 1) == 0)
                    de.blocks.remove(de.blocks.size() - 1);

                int length = de.blocks.size() * geometry.blockSize;
                ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
                attrs.put(Attributes.FILENAME, de.filename);
                attrs.put(Attributes.LENGTH, Integer.toString(length));
                attrs.put(Attributes.FILE_TYPE, "file");
                attrs.put(Attributes.MODE, "");

                de.dirent = Dirent
                        .builder()
                        .setPath(VfsPath.of("/").resolve(de.filename))
                        .setFilename(de.filename)
                        .setLength(length)
                        .setMode("")
                        .setFileType(IS_FILE)
                        .setAttributes(attrs.build())
                        .build();
            }

            // allocation bitmap at 0xa00
            br.seek(0xa00);
            int used = 0;
            for (int i = 0; i < geometry.filesystemBlocks; i++)
            {
                if (br.eof())
                    break;
                int v = br.read8() & 0xff;
                if (v != 0)
                    used++;
            }
            usedBlocks = used;
            totalBlocks = geometry.filesystemBlocks;
        }

        RolandEntry findFile(String wanted) throws IOException
        {
            RolandEntry de = map.get(wanted);
            if (de == null)
                throw new NoSuchFileException(wanted);
            return de;
        }
    }
}
