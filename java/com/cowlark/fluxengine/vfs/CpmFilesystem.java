package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import org.indunet.fastproto.FastProto;
import org.indunet.fastproto.annotation.UInt16ArrayType;
import org.indunet.fastproto.annotation.UInt8ArrayType;
import org.indunet.fastproto.annotation.UInt8Type;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

public class CpmFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final CpmFsProto config;
    private final BlockDevice blockDevice;
    private long filesystemStartOffset;
    private int sectorSize;
    private int blockSectors;
    private int recordsPerBlock;
    private int dirBlocks;
    private int filesystemBlocks;
    private int allocationMapSize;
    private int logicalExtentsPerEntry;
    private int logicalExtentMask;
    private int blocksPerLogicalExtent;
    private Bytes directory;
    private boolean mounted;

    public CpmFilesystem(CpmFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private int computeSector(int block)
    {
        int sector = block * blockSectors;
        if (config.hasPadding())
        {
            CpmFsProto.Padding padding = config.getPadding();
            int every = padding.getEvery();
            int amount = padding.getAmount();
            if (every != 0)
                sector += (sector / every) * amount;
        }
        return sector;
    }

    private Bytes getCpmBlock(int block) throws IOException
    {
        int sector = computeSector(block);
        long offset = filesystemStartOffset + (long) sector * sectorSize;
        return blockDevice.getBytes((int) offset, config.getBlockSize());
    }

    private Bytes getCpmBlocks(int startBlock, int count) throws IOException
    {
        Bytes result = new Bytes();
        ByteWriter bw = new ByteWriter(result);
        for (int i = 0; i < count; i++)
            bw.write(getCpmBlock(startBlock + i));
        return result;
    }

    private void mount() throws IOException
    {
        CpmFsProto.Location start = config.getFilesystemStart();
        int track = start.getTrack();
        int side = start.getSide();
        int sectorId = start.getSector();

        CylinderHeadSector loc = new CylinderHeadSector(track, side, sectorId);
        Long off = blockDevice.diskLayout.sectorOffsetByLogicalSectorLocation.get(loc);
        if (off == null)
        {
            long min = Long.MAX_VALUE;
            Long found = null;
            for (Map.Entry<CylinderHeadSector, Long> e :
                    blockDevice.diskLayout.sectorOffsetByLogicalSectorLocation.entrySet())
            {
                CylinderHeadSector ll = e.getKey();
                if (ll.cylinder() == track && ll.head() == side)
                {
                    long v = e.getValue();
                    if (v < min)
                    {
                        min = v;
                        found = v;
                    }
                }
            }
            if (found != null)
                off = found;
            else
                throw new FileSystemException(
                        "Invalid filesystem: filesystem_start not found: " + loc);
        }
        filesystemStartOffset = off;

        CylinderHead ch = new CylinderHead(track, side);
        LogicalTrackLayout ltl = blockDevice.diskLayout.layoutByLogicalLocation.get(ch);
        if (ltl == null)
            ltl = blockDevice.diskLayout.layoutByLogicalLocation.get(new CylinderHead(0, 0));
        if (ltl == null)
            throw new FileSystemException("Invalid filesystem");

        sectorSize = ltl.sectorSize;
        if (sectorSize == 0)
            throw new FileSystemException("Invalid filesystem");

        int blockSize = config.getBlockSize();
        if (blockSize == 0)
            throw new FileSystemException("Invalid filesystem: block_size is 0");

        blockSectors = blockSize / sectorSize;
        recordsPerBlock = blockSize / 128;
        dirBlocks = (config.getDirEntries() * 32) / blockSize;

        long totalBytes = blockDevice.diskLayout.totalBytes;
        filesystemBlocks = (int) ((totalBytes - filesystemStartOffset) / blockSize);
        allocationMapSize = (filesystemBlocks < 256) ? 1 : 2;

        int physicalExtentSize;
        if (allocationMapSize == 1)
            physicalExtentSize = blockSize * 16;
        else
            physicalExtentSize = blockSize * 8;

        logicalExtentsPerEntry = physicalExtentSize / 16384;
        logicalExtentMask = logicalExtentsPerEntry - 1;
        blocksPerLogicalExtent = 16384 / blockSize;

        directory = getCpmBlocks(0, dirBlocks);
        mounted = true;
    }

    private void ensureMounted() throws IOException
    {
        if (!mounted)
            mount();
        else
        {
            // Re-mount to pick up any external changes? For read-only this is fine to
            // recompute each time, but we keep the flag to avoid overhead.
            // Simpler: always remount.
            mount();
        }
    }

    private Entry getEntry(int d)
    {
        Bytes bytes = directory.slice(d * 32, 32);
        return new Entry(bytes, allocationMapSize, d);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        ensureMounted();

        int usedBlocks = dirBlocks;
        for (int d = 0; d < config.getDirEntries(); d++)
        {
            Entry entry = getEntry(d);
            if (entry.deleted)
                continue;
            for (int block : entry.allocationMap)
                if (block != 0)
                    usedBlocks++;
        }

        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(FilesystemAttributes.VOLUME_NAME.name(), "");
        builder.put(FilesystemAttributes.TOTAL_BLOCKS.name(), Integer.toString(filesystemBlocks));
        builder.put(FilesystemAttributes.USED_BLOCKS.name(), Integer.toString(usedBlocks));
        builder.put(
                FilesystemAttributes.BLOCK_SIZE.name(),
                Integer.toString(config.getBlockSize()));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        ensureMounted();

        Map<String, Integer> lengthMap = new LinkedHashMap<>();
        Map<String, String> modeMap = new LinkedHashMap<>();

        for (int d = 0; d < config.getDirEntries(); d++)
        {
            Entry entry = getEntry(d);
            if (entry.deleted)
                continue;

            String combined = entry.combinedFilename();
            int length = entry.extent * 16384 + entry.records * 128;
            Integer existing = lengthMap.get(combined);
            if (existing == null || length > existing)
                lengthMap.put(combined, length);
            if (!modeMap.containsKey(combined))
                modeMap.put(combined, entry.mode);
        }

        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (Map.Entry<String, Integer> e : lengthMap.entrySet())
        {
            String combined = e.getKey();
            int length = e.getValue();
            String mode = modeMap.get(combined);

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(FileAttributes.FILENAME.name(), combined);
            attrs.put(FileAttributes.LENGTH.name(), Integer.toString(length));
            attrs.put(FileAttributes.FILE_TYPE.name(), "file");
            attrs.put(FileAttributes.MODE.name(), mode);

            Dirent dirent = Dirent
                    .builder()
                    .setPath(VfsPath.of("/").resolve(combined))
                    .setFilename(combined)
                    .setLength(length)
                    .setMode(mode)
                    .setFileType(IS_FILE)
                    .setAttributes(attrs.build())
                    .build();
            builder.put(combined, dirent);
        }
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.isRoot())
            return ROOT_DIRENT;
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        ImmutableMap<String, Dirent> map = list(VfsPath.root());
        String wanted = path.segments().get(0);
        Dirent dirent = map.get(wanted);
        if (dirent == null)
            throw new NoSuchFileException(path.toString());
        return dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        ensureMounted();

        String wanted = path.segments().get(0);
        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        int logicalExtent = 0;

        for (; ; )
        {
            Entry found = null;
            for (int d = 0; d < config.getDirEntries(); d++)
            {
                Entry entry = getEntry(d);
                if (entry.deleted)
                    continue;
                if (!wanted.equals(entry.combinedFilename()))
                    continue;
                if (entry.extent < logicalExtent)
                    continue;
                if ((entry.extent & ~logicalExtentMask) == (logicalExtent & ~logicalExtentMask))
                {
                    found = entry;
                    break;
                }
            }

            if (found == null)
            {
                if (logicalExtent == 0)
                    throw new NoSuchFileException(path.toString());
                break;
            }

            int i = (logicalExtent & logicalExtentMask) * blocksPerLogicalExtent;
            int records;
            if (found.extent == logicalExtent)
                records = found.records;
            else
                records = 128;

            while (records != 0)
            {
                int blockId = 0;
                if (i < found.allocationMap.size())
                    blockId = found.allocationMap.get(i);

                Bytes block;
                if (blockId != 0)
                    block = getCpmBlock(blockId);
                else
                {
                    block = new Bytes(config.getBlockSize());
                }

                int r = Math.min(records, recordsPerBlock);
                bw.write(block.slice(0, r * 128));
                records -= r;
                i++;
            }

            logicalExtent++;
        }

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

    private static class Entry
    {
        int index;
        String filename;
        String mode;
        int user;
        int extent;
        int records;
        List<Integer> allocationMap;
        boolean deleted;

        Entry(Bytes bytes, int mapEntrySize, int index)
        {
            this.index = index;

            int st;
            int[] f;
            int xl, bc, xh;

            switch (mapEntrySize)
            {
                case 1 ->
                {
                    Cpm8Dirent de = FastProto.decode(bytes.toByteArray(), Cpm8Dirent.class);
                    st = de.st;
                    f = de.f;
                    xl = de.xl;
                    bc = de.bc;
                    xh = de.xh;
                    records = de.rc;
                    allocationMap = de.al;
                }

                case 2 ->
                {
                    Cpm16Dirent de = FastProto.decode(bytes.toByteArray(), Cpm16Dirent.class);
                    st = de.st;
                    f = de.f;
                    xl = de.xl;
                    bc = de.bc;
                    xh = de.xh;
                    records = de.rc;
                    allocationMap = de.al;
                }

                default -> throw new IllegalArgumentException("Bad map entry size " + mapEntrySize);
            }

            if (st == 0xe5)
                deleted = true;
            user = st & 0x0f;

            {
                StringBuilder ss = new StringBuilder();
                for (int i = 0; i < 7; i++)
                {
                    int c = f[i] & 0x7f;
                    if (c == ' ')
                        break;
                    ss.append((char) c);
                }
                for (int i = 8; i <= 10; i++)
                {
                    int c = f[i] & 0x7f;
                    if (c == ' ')
                        break;
                    if (i == 8)
                        ss.append('.');
                    ss.append((char) c);
                }
                filename = ss.toString();
            }

            {
                StringBuilder ss = new StringBuilder();
                if ((f[8] & 0x80) != 0)
                    ss.append('R');
                if ((f[9] & 0x80) != 0)
                    ss.append('S');
                if ((f[10] & 0x80) != 0)
                    ss.append('A');
                mode = ss.toString();
            }

            extent = xl | (xh << 5);
        }

        String combinedFilename()
        {
            if (user == 0)
                return filename;
            return String.format("%d:%s", user, filename);
        }
    }

    public static class Cpm8Dirent
    {
        @UInt8Type(offset = 0) public int st;
        @UInt8ArrayType(offset = 1, length = 11) public int[] f;
        @UInt8Type(offset = 12) public int xl;
        @UInt8Type(offset = 13) public int bc;
        @UInt8Type(offset = 14) public int xh;
        @UInt8Type(offset = 15) public int rc;
        @UInt8ArrayType(offset = 16, length = 16) public ArrayList<Integer> al;
    }

    public static class Cpm16Dirent
    {
        @UInt8Type(offset = 0) public int st;
        @UInt8ArrayType(offset = 1, length = 11) public int[] f;
        @UInt8Type(offset = 12) public int xl;
        @UInt8Type(offset = 13) public int bc;
        @UInt8Type(offset = 14) public int xh;
        @UInt8Type(offset = 15) public int rc;
        @UInt16ArrayType(offset = 16, length = 8) public ArrayList<Integer> al;
    }
}
