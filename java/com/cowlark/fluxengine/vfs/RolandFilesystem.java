package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_CREATE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_DELETE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_PUTFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;
import static com.google.common.collect.ImmutableList.toImmutableList;
import static com.google.common.collect.ImmutableListMultimap.toImmutableListMultimap;
import static com.google.common.collect.ImmutableMap.toImmutableMap;
import static java.util.Arrays.stream;
import static org.apache.commons.lang3.StringUtils.stripEnd;
import static org.apache.commons.lang3.StringUtils.substring;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.google.common.base.Strings;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.io.Files;
import org.apache.commons.lang3.ArrayUtils;
import org.indunet.fastproto.FastProto;
import org.indunet.fastproto.annotation.DecodingFormula;
import org.indunet.fastproto.annotation.EncodingFormula;
import org.indunet.fastproto.annotation.Expect;
import org.indunet.fastproto.annotation.StringType;
import org.indunet.fastproto.annotation.StructArrayType;
import org.indunet.fastproto.annotation.UInt8ArrayType;
import org.indunet.fastproto.annotation.UInt8Type;
import org.indunet.fastproto.exception.DecodingException;
import org.indunet.fastproto.exception.EncodingException;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.Comparator;
import java.util.function.Function;

public class RolandFilesystem extends Filesystem
{
    public final static int NUM_DIRECTORY_ENTRIES = 79;
    public final static int SECTOR_SIZE = 256;
    public final static int BLOCK_SIZE = 12 * SECTOR_SIZE; /* one track */
    public final static int BLOCK_SECTORS = BLOCK_SIZE / SECTOR_SIZE;
    public final static int DIRECTORY_TRACK = 39;
    public final static int DIRECTORY_SECTOR = DIRECTORY_TRACK * BLOCK_SECTORS;
    public final static int EXTENT_BLOCKS = 16;

    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of(
            OP_GETFSDATA,
            OP_CREATE,
            OP_LIST,
            OP_DELETE,
            OP_GETFILE,
            OP_PUTFILE,
            OP_GETDIRENT);

    private final RolandFsProto config;
    private final BlockDevice blockDevice;
    private Geometry geometry = null;

    public RolandFilesystem(RolandFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private static String getFilename(VfsPath path)
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");
        return path.segments().getFirst();
    }

    private static boolean isFile(RolandDirent de)
    {
        return (de.flag == 0) && !de.filename.isEmpty();
    }

    private static ImmutableList<RolandDirent> findFileFor(
            RolandDirectory directory,
            String filename)
    {
        return stream(directory.dirents)
                .filter(dirent -> isFile(dirent) && dirent.filename.equals(filename))
                .collect(toImmutableList());
    }

    private static ImmutableList<RolandDirent> findExistingFileFor(
            RolandDirectory directory,
            String filename) throws NoSuchFileException
    {
        ImmutableList<RolandDirent> des = findFileFor(directory, filename);
        if (des.isEmpty())
            throw new NoSuchFileException(filename);
        return des;
    }

    private static int allocateBlock(RolandDirectory dir) throws FileSystemException
    {
        int firstFreeBlock = ArrayUtils.indexOf(dir.allocationBitmap, 0);
        if (firstFreeBlock == -1)
            throw new FileSystemException("no room");
        dir.allocationBitmap[firstFreeBlock] = 0xff;
        return firstFreeBlock;
    }

    private static RolandDirent allocateDirent(RolandDirectory dir, String filename, int extent)
            throws FileSystemException
    {
        RolandDirent firstFreeDirent = stream(dir.dirents)
                .filter(de -> !isFile(de))
                .findFirst()
                .orElseThrow(() -> new FileSystemException("catalogue full"));
        firstFreeDirent.flag = 0;
        firstFreeDirent.extent = extent;
        firstFreeDirent.filename = filename;
        firstFreeDirent.blocks = new int[EXTENT_BLOCKS];
        return firstFreeDirent;
    }

    private Geometry computeGeometry()
    {
        Geometry g = new Geometry();
        int totalSectors = blockDevice.getBlockCount();
        g.filesystemBlocks = totalSectors / BLOCK_SECTORS;
        g.directoryByteOffset = DIRECTORY_TRACK * BLOCK_SECTORS * SECTOR_SIZE;
        g.midBlock = (totalSectors - DIRECTORY_SECTOR) / BLOCK_SECTORS;
        return g;
    }

    private int blockToLogicalSectorNumber(int block)
    {
        int track;
        if (block < geometry.midBlock)
            track = DIRECTORY_TRACK + block;
        else
            track = DIRECTORY_TRACK - (1 + block - geometry.midBlock);
        return track * BLOCK_SECTORS;
    }

    private Bytes getRolandBlock(int number) throws IOException
    {
        int lba = blockToLogicalSectorNumber(number);
        return blockDevice.getBlocks(lba, BLOCK_SECTORS);
    }

    private void putRolandBlock(int number, Bytes data) throws IOException
    {
        int lba = blockToLogicalSectorNumber(number);
        blockDevice.putBlocks(lba, data);
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        geometry = computeGeometry();
        RolandDirectory dir = new RolandDirectory();
        dir.numBlocks = geometry.filesystemBlocks;
        dir.allocationBitmap = new int[geometry.filesystemBlocks];
        dir.allocationBitmap[0] = 0xff;
        dir.dirents = new RolandDirent[NUM_DIRECTORY_ENTRIES];
        for (int i = 0; i < NUM_DIRECTORY_ENTRIES; i++)
        {
            RolandDirent de = new RolandDirent();
            de.flag = 0xe5;
            dir.dirents[i] = de;
        }
        unmount(dir);
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
        return stream(dir.dirents)
                .filter(RolandFilesystem::isFile)
                .collect(toImmutableListMultimap(de -> de.filename, de -> de))
                .asMap()
                .values()
                .stream()
                .map(des -> getDirentFor((ImmutableCollection<RolandDirent>) des))
                .collect(toImmutableMap(Dirent::filename, de -> de));
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        var filename = getFilename(path);

        RolandDirectory dir = mount();
        return getDirentFor(findExistingFileFor(dir, filename));
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        String filename = getFilename(path);

        RolandDirectory dir = mount();
        ImmutableList<RolandDirent> des = findExistingFileFor(dir, filename);

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (RolandDirent de : des)
        {
            for (int i = 0; i < de.blocks.length; i++)
            {
                int blockNumber = de.blocks[i];
                if (blockNumber != 0)
                {
                    Bytes blockData = getRolandBlock(blockNumber);
                    bw.seek((de.extent * 16 + i) * BLOCK_SIZE);
                    bw.write(blockData);
                }
            }
        }
        return data;
    }

    @Override
    public void putFile(VfsPath path, Bytes bytes) throws IOException
    {
        String filename = getFilename(path);

        RolandDirectory dir = mount();
        try
        {
            deleteDes(dir, findExistingFileFor(dir, filename));
        } catch (NoSuchFileException e)
        {
        }

        int oldExtent = -1;
        RolandDirent de = null;
        ByteReader br = new ByteReader(bytes);
        do
        {
            int blockCount = br.pos() / BLOCK_SIZE;
            int newExtent = blockCount / EXTENT_BLOCKS;
            if (newExtent != oldExtent)
            {
                de = allocateDirent(dir, filename, newExtent);
                oldExtent = newExtent;
            }

            int blockIndexInExtent = blockCount % EXTENT_BLOCKS;
            Bytes data = br.readPadded(BLOCK_SIZE);
            int blockNumber = allocateBlock(dir);
            de.blocks[blockIndexInExtent] = blockNumber;
            putRolandBlock(blockNumber, data);
        } while (!br.eof());

        unmount(dir);
    }

    @Override
    public void deleteFile(VfsPath path) throws IOException
    {
        String filename = getFilename(path);

        RolandDirectory dir = mount();
        deleteDes(dir, findExistingFileFor(dir, filename));
        unmount(dir);
    }

    private void deleteDes(RolandDirectory dir, ImmutableCollection<RolandDirent> des)
    {
        for (RolandDirent de : des)
        {
            de.flag = 0xe5;
            for (int block : de.blocks)
                if (block != 0)
                    dir.allocationBitmap[block] = 0;
            de.blocks = new int[EXTENT_BLOCKS];
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
        blockDevice.commit();
    }

    @Override
    public void discardChanges() throws IOException
    {
        blockDevice.revert();
    }

    private Dirent getDirentFor(ImmutableCollection<RolandDirent> de)
    {
        RolandDirent lastDe = de.stream().max(Comparator.comparing(d -> d.extent)).get();
        int lastBlockCount = ArrayUtils.indexOf(lastDe.blocks, 0);
        if (lastBlockCount == -1)
            lastBlockCount = EXTENT_BLOCKS;

        int length = (lastDe.extent * EXTENT_BLOCKS + lastBlockCount) * BLOCK_SIZE;
        ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
        attrs.put(Attributes.FILENAME, lastDe.filename);
        attrs.put(Attributes.LENGTH, Integer.toString(length));
        attrs.put(Attributes.FILE_TYPE, "file");

        return Dirent
                .builder()
                .setPath(VfsPath.of("/").resolve(lastDe.filename))
                .setFilename(lastDe.filename)
                .setLength(length)
                .setMode("")
                .setFileType(IS_FILE)
                .setAttributes(attrs.build())
                .build();
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

        try
        {
            return FastProto.decode(directory.toByteArray(), RolandDirectory.class);
        } catch (DecodingException e)
        {
            throw new FluxEngineFileSystemException("Invalid filesystem", e);
        }
    }

    private void unmount(RolandDirectory dir) throws IOException
    {
        try
        {
            Bytes bytes = new Bytes(FastProto.encode(dir));
            putRolandBlock(0, bytes);
        } catch (EncodingException e)
        {
            throw new FluxEngineFileSystemException("Invalid filesystem", e);
        }
    }

    private static class Geometry
    {
        int filesystemBlocks;
        int midBlock;
        int directoryByteOffset;
    }

    public static class RolandDirent
    {
        @UInt8Type(offset = 0) public int flag;

        @EncodingFormula(FilenameEncoder.class) @DecodingFormula(FilenameDecoder.class)
        @StringType(offset = 1, length = 13) public String filename;

        @UInt8Type(offset = 15) public int extent;
        @UInt8ArrayType(offset = 16, length = 16) public int[] blocks;

        public static class FilenameDecoder implements Function<String, String>
        {
            @Override
            public String apply(String s)
            {
                String base = stripEnd(substring(s, 0, 10), "_ \0");
                String ext = stripEnd(substring(s, 10), "_ \0");
                if (base.isEmpty())
                    return "";
                if (ext.isEmpty())
                    return base;
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

                return Strings.padEnd(base, 10, '_') + Strings.padEnd(ext, 3, '_');
            }
        }
    }

    public static class RolandDirectory
    {
        @Expect(offset = 1,
                bytes = {'R', 'O', 'L', 'A', 'N', 'D', '-', 'G', 'C', 'R', 'D', 'O', 'S'})
        public transient int _magic;

        @UInt8Type(offset = 14) public int numBlocks;

        @StructArrayType(offset = 32, length = NUM_DIRECTORY_ENTRIES, element = RolandDirent.class)
        public RolandDirent[] dirents;

        @UInt8ArrayType(offset = 0xa00, lengthRef = "$numBlocks") public int[] allocationBitmap;
    }

    record BlockUsage(int totalBlocks, int usedBlocks)
    {
    }
}
