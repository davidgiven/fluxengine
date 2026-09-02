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
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class LifFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private static final ImmutableMap<Integer, String> NUMBER_TO_FILE_TYPE =
            ImmutableMap.<Integer, String>builder()
                    .put(0x0001, "TEXT")
                    .put(0x00ff, "D-LEX")
                    .put(0xe008, "BIN8x")
                    .put(0xe010, "DTA8x")
                    .put(0xe020, "BAS8x")
                    .put(0xe030, "XM41")
                    .put(0xe040, "ALL41")
                    .put(0xe050, "KEY41")
                    .put(0xe052, "TXT75")
                    .put(0xe053, "APP75")
                    .put(0xe058, "DAT75")
                    .put(0xe060, "STA41")
                    .put(0xe070, "X-M41")
                    .put(0xe080, "PGM41")
                    .put(0xe088, "BAS75")
                    .put(0xe089, "LEX75")
                    .put(0xe08a, "WKS75")
                    .put(0xe08b, "ROM75")
                    .put(0xe0d0, "SDATA")
                    .put(0xe0d1, "TEXT_S")
                    .put(0xe0f0, "DAT71")
                    .put(0xe0f1, "DAT71_S")
                    .put(0xe204, "BIN71")
                    .put(0xe205, "BIN71_S")
                    .put(0xe206, "BIN71_P")
                    .put(0xe207, "BIN71_SP")
                    .put(0xe208, "LEX71")
                    .put(0xe209, "LEX71_S")
                    .put(0xe20a, "LEX71_P")
                    .put(0xe20b, "LEX71_SP")
                    .put(0xe20c, "KEY71")
                    .put(0xe20d, "KEY71_S")
                    .put(0xe214, "BAS71")
                    .put(0xe215, "BAS71_S")
                    .put(0xe216, "BAS71_P")
                    .put(0xe217, "BAS71_SP")
                    .put(0xe218, "FTH71")
                    .put(0xe219, "FTH71_S")
                    .put(0xe21a, "FTH71_P")
                    .put(0xe21b, "FTH71_SP")
                    .put(0xe21c, "ROM71")
                    .put(0xe222, "GRA71")
                    .put(0xe224, "ADR71")
                    .put(0xe22e, "SYM71")
                    .put(0xe942, "SYS9k")
                    .put(0xe946, "HP-UX")
                    .put(0xe950, "BAS9k")
                    .put(0xe961, "BDA9k")
                    .put(0xe971, "BIN9k")
                    .put(0xea0a, "DTA9k")
                    .put(0xea32, "COD9k")
                    .put(0xea3e, "TXT9k")
                    .build();

    private final LifProto config;
    private final BlockDevice blockDevice;
    private final int lifBlockSize;

    private static String trimWhitespace(String s)
    {
        // Mirrors C++ WHITESPACE " \t\n\r\f\v\0"
        int start = 0;
        int end = s.length();
        while (start < end && isWhitespace(s.charAt(start)))
            start++;
        while (end > start && isWhitespace(s.charAt(end - 1)))
            end--;
        return s.substring(start, end);
    }

    private static boolean isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'
                || c == '\u000B' || c == '\0';
    }

    private static String bytesToString(Bytes bytes)
    {
        byte[] arr = bytes.toByteArray();
        return new String(arr, StandardCharsets.ISO_8859_1);
    }

    private static int unbcd(int b)
    {
        return ((b >> 4) & 0x0f) * 10 + (b & 0x0f);
    }

    private static class LifDirent
    {
        String filename;
        int location;
        int length;
        String ctime;
        String mode;
        Dirent dirent;

        LifDirent(Bytes bytes, int lifBlockSize)
        {
            ByteReader br = new ByteReader(bytes);
            Bytes filenameBytes = br.read(10);
            filename = trimWhitespace(bytesToString(filenameBytes));

            int type = br.readBe16() & 0xffff;
            location = br.readBe32();
            int blocks = br.readBe32();
            length = blocks * lifBlockSize;

            int year = unbcd(br.read8() & 0xff);
            int month = unbcd(br.read8() & 0xff) + 1;
            int day = unbcd(br.read8() & 0xff);
            int hour = unbcd(br.read8() & 0xff);
            int minute = unbcd(br.read8() & 0xff);
            int second = unbcd(br.read8() & 0xff);
            int volume = br.readBe16() & 0xffff;
            int protection = br.readBe16() & 0xffff;
            int recordSize = br.readBe16() & 0xffff;

            if (year >= 70)
                year += 1900;
            else
                year += 2000;

            ctime = String.format(
                    "%04d-%02d-%02dT%02d:%02d:%02d+0000",
                    year, month, day, hour, minute, second);

            String typeStr = NUMBER_TO_FILE_TYPE.get(type);
            if (typeStr != null)
                mode = typeStr;
            else
                mode = String.format("0x%04x", type);

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put("lif.ctime", ctime);
            attrs.put("lif.volume", Integer.toString(volume & 0x7fff));
            attrs.put("lif.protection", String.format("0x%x", protection));
            attrs.put("lif.record_size", Integer.toString(recordSize));

            dirent = Dirent.builder()
                    .setPath(VfsPath.of("/").resolve(filename))
                    .setFilename(filename)
                    .setLength(length)
                    .setMode(mode)
                    .setFileType(IS_FILE)
                    .setAttributes(attrs.build())
                    .build();
        }
    }

    private class LifDirectory
    {
        String volumeLabel;
        int directoryBlock;
        int directorySize;
        int totalBlocks;
        int usedBlocks;
        List<LifDirent> dirents = new ArrayList<>();

        LifDirectory() throws IOException
        {
            Bytes rootBlock = getLifBlock(0);
            ByteReader rbr = new ByteReader(rootBlock);
            int magic = rbr.readBe16() & 0xffff;
            if (magic != 0x8000)
                throw new FileSystemException("Invalid filesystem");

            Bytes labelBytes = rbr.read(6);
            volumeLabel = trimWhitespace(bytesToString(labelBytes));

            directoryBlock = rbr.readBe32();
            rbr.skip(4);
            directorySize = rbr.readBe32();
            rbr.skip(4);
            int tracks = rbr.readBe32();
            int heads = rbr.readBe32();
            int sectors = rbr.readBe32();

            usedBlocks = 1 + directorySize;

            Bytes directory = getLifBlock(directoryBlock, directorySize);
            ByteReader br = new ByteReader(directory);
            while (!br.eof())
            {
                Bytes direntBytes = br.read(32);
                if ((direntBytes.getByte(0) & 0xff) != 0xff)
                {
                    LifDirent de = new LifDirent(direntBytes, lifBlockSize);
                    usedBlocks += de.length / lifBlockSize;
                    dirents.add(de);
                }
            }

            int geometryBlocks = tracks * heads * sectors;
            totalBlocks = Math.max(geometryBlocks, usedBlocks);
        }

        LifDirent findFile(String wanted) throws IOException
        {
            for (LifDirent de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }
            throw new NoSuchFileException(wanted);
        }
    }

    public LifFilesystem(LifProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        int bs = 256;
        if (config != null)
        {
            // proto2 default is 256, but guard against 0
            int v = config.getBlockSize();
            if (v != 0)
                bs = v;
        }
        this.lifBlockSize = bs;
    }

    private int getLogicalSectorSize()
    {
        return blockDevice.diskLayout.layoutByLogicalLocation
                .get(new CylinderHead(0, 0))
                .sectorSize;
    }

    private Bytes getLifBlock(int number) throws IOException
    {
        int sectorSize = getLogicalSectorSize();
        int sectorsPerBlock = sectorSize / lifBlockSize;
        if (sectorsPerBlock <= 0)
            sectorsPerBlock = 1;
        int sectorNumber = number / sectorsPerBlock;
        int offset = number % sectorsPerBlock;
        Bytes sector = blockDevice.getBlock(sectorNumber);
        return sector.slice(offset * lifBlockSize, lifBlockSize);
    }

    private Bytes getLifBlock(int number, int count) throws IOException
    {
        Bytes b = new Bytes();
        ByteWriter bw = new ByteWriter(b);
        while (count > 0)
        {
            bw.write(getLifBlock(number));
            number++;
            count--;
        }
        return b;
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        LifDirectory dir = new LifDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, dir.volumeLabel);
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(dir.totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(lifBlockSize));
        builder.put("lif.directory_block", Integer.toString(dir.directoryBlock));
        builder.put("lif.directory_size", Integer.toString(dir.directorySize));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        LifDirectory dir = new LifDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (LifDirent de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        LifDirectory dir = new LifDirectory();
        return dir.findFile(path.segments().get(0)).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        LifDirectory dir = new LifDirectory();
        LifDirent de = dir.findFile(path.segments().get(0));
        return getLifBlock(de.location, de.length / lifBlockSize);
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
