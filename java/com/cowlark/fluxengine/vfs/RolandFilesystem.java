package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import org.apache.commons.lang3.ArrayUtils;
import org.apache.commons.lang3.StringUtils;
import org.indunet.fastproto.FastProto;
import org.indunet.fastproto.annotation.BinaryType;
import org.indunet.fastproto.annotation.DecodingFormula;
import org.indunet.fastproto.annotation.EncodingFormula;
import org.indunet.fastproto.annotation.Expect;
import org.indunet.fastproto.annotation.StringType;
import org.indunet.fastproto.annotation.StructArrayType;
import org.indunet.fastproto.annotation.UInt8Type;
import org.indunet.fastproto.exception.DecodingException;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.function.Function;

public class RolandFilesystem extends Filesystem
{
    public final static int NUM_DIRECTORY_ENTRIES = 79;
    public final static int SECTOR_SIZE = 256;
    public final static int BLOCK_SIZE = 12 * SECTOR_SIZE; /* one track */
    public final static int BLOCK_SECTORS = BLOCK_SIZE / SECTOR_SIZE;

    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final RolandFsProto config;
    private final BlockDevice blockDevice;
    private Geometry geometry = null;

    public RolandFilesystem(RolandFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private int getOffsetOfSector(int track, int side, int sector) throws IOException
    {
        CylinderHeadSector loc = new CylinderHeadSector(track, side, sector);
        Long offset = blockDevice.diskLayout.sectorOffsetByLogicalSectorLocation.get(loc);
        if (offset == null)
            throw new FileSystemException("sector not found: " + loc);
        return offset.intValue();
    }

    private Geometry computeGeometry()
    {
        Geometry g = new Geometry();
        g.directoryTrack = config.getDirectoryTrack();
        int totalSectors = blockDevice.getBlockCount();
        g.filesystemBlocks = totalSectors / BLOCK_SECTORS;
        try
        {
            g.directoryByteOffset = getOffsetOfSector(g.directoryTrack, 0, 0);
        } catch (IOException e)
        {
            // fallback to linear calculation if layout lookup fails
            g.directoryByteOffset = g.directoryTrack * BLOCK_SECTORS * SECTOR_SIZE;
        }
        g.directorySector = g.directoryByteOffset / SECTOR_SIZE;
        g.midBlock = (totalSectors - g.directorySector) / BLOCK_SECTORS;
        return g;
    }

    private int blockToLogicalSectorNumber(int block)
    {
        int track;
        if (block < geometry.midBlock)
            track = geometry.directoryTrack + block;
        else
            track = geometry.directoryTrack - (1 + block - geometry.midBlock);
        return track * BLOCK_SECTORS;
    }

    private Bytes getRolandBlock(int number) throws IOException
    {
        int lba = blockToLogicalSectorNumber(number);
        return blockDevice.getBlocks(lba, BLOCK_SECTORS);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        RolandDirectory dir = mount();
        BlockUsage blockUsage = countBlocks(dir);
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(blockUsage.totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(blockUsage.usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(BLOCK_SIZE));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        RolandDirectory dir = mount();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (RolandDirent de : dir.dirents)
            if ((de.flag == 0) && (de.filename.length() > 0))
                builder.put(de.filename, getDirentFor(de));
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        RolandDirectory dir = mount();
        String wanted = path.segments().get(0);
        return getDirentFor(findFileFor(dir, wanted));
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        RolandDirectory dir = mount();
        String wanted = path.segments().get(0);
        RolandDirent de = findFileFor(dir, wanted);

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (int blockNum : de.blocks)
        {
            Bytes blockData = getRolandBlock(blockNum);
            bw.write(blockData);
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

    private Dirent getDirentFor(RolandDirent de)
    {
        int blockCount = ArrayUtils.indexOf(de.blocks, (byte) 0);
        if (blockCount == -1)
            blockCount = 16;

        int length = blockCount * BLOCK_SIZE;
        ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
        attrs.put(Attributes.FILENAME, de.filename);
        attrs.put(Attributes.LENGTH, Integer.toString(length));
        attrs.put(Attributes.FILE_TYPE, "file");
        attrs.put(Attributes.MODE, "");

        return Dirent
                .builder()
                .setPath(VfsPath.of("/").resolve(de.filename))
                .setFilename(de.filename)
                .setLength(length)
                .setMode("")
                .setFileType(IS_FILE)
                .setAttributes(attrs.build())
                .build();
    }

    public RolandFilesystem.RolandDirent findFileFor(
            RolandFilesystem.RolandDirectory directory,
            String filename) throws NoSuchFileException
    {
        for (var dirent : directory.dirents)
        {
            if ((dirent.flag == 0) && dirent.filename.equals(filename))
                return dirent;
        }
        throw new NoSuchFileException(filename);
    }

    private BlockUsage countBlocks(RolandDirectory directory)
    {
        int totalBlocks = 0;
        int usedBlocks = 0;
        for (int i = 0; i < geometry.filesystemBlocks; i++)
        {
            totalBlocks++;
            if (directory.allocationBitmap[i] != 0)
                usedBlocks++;
        }
        return new BlockUsage(totalBlocks, usedBlocks);
    }

    private RolandDirectory mount() throws IOException
    {
        geometry = computeGeometry();
        Bytes directory = getRolandBlock(0);
        if (directory.size() < 32)
            throw new FileSystemException("Invalid filesystem");

        try
        {
            return FastProto.decode(directory.toByteArray(), RolandDirectory.class);
        } catch (DecodingException e)
        {
            throw new FluxEngineFileSystemException("Invalid filesystem", e);
        }
    }

    private static class Geometry
    {
        int directoryTrack;
        int filesystemBlocks;
        int midBlock;
        int directoryByteOffset;
        int directorySector;
    }

    public static class RolandDirent
    {
        @UInt8Type(offset = 0) public int flag;

        @EncodingFormula(FilenameEncoder.class) @DecodingFormula(FilenameDecoder.class)
        @StringType(offset = 1, length = 13) public String filename;
        @BinaryType(offset = 16, length = 16) public byte[] blocks;

        public static class FilenameDecoder implements Function<String, String>
        {
            @Override
            public String apply(String s)
            {
                String base = StringUtils.stripEnd(s.substring(0, 10), "_ \0");
                String ext = s.substring(10);
                if (base.isEmpty())
                    return "";
                return base + "." + ext;
            }
        }

        public static class FilenameEncoder implements Function<String, String>
        {
            @Override
            public String apply(String s)
            {
                String base = Files.getNameWithoutExtension(s);
                String ext = Files.getFileExtension(s);

                return Strings.padEnd(base, 10, '_') + ext;
            }
        }
    }

    public static class RolandDirectory
    {
        @Expect(offset = 1,
                bytes = {'R', 'O', 'L', 'A', 'N', 'D', '-', 'G', 'C', 'R', 'D', 'O', 'S'})
        public transient int _magic;

        @StructArrayType(offset = 32, length = NUM_DIRECTORY_ENTRIES, element = RolandDirent.class)
        public RolandDirent[] dirents;

        @BinaryType(offset = 0xa00, length = 512) public byte[] allocationBitmap;
    }

    record BlockUsage(int totalBlocks, int usedBlocks)
    {
    }
}
