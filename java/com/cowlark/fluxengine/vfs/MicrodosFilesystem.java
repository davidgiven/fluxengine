package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class MicrodosFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private static final int BLOCK_SIZE = 512;
    private static final int TOTAL_BLOCKS = 630;

    private final MicrodosProto config;
    private final BlockDevice blockDevice;

    private static class Sdw
    {
        final int start;
        final int length;

        Sdw(int start, int length)
        {
            this.start = start;
            this.length = length;
        }
    }

    private static boolean isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f'
                || c == '\u000B' || c == '\0';
    }

    private static String trimWhitespace(String s)
    {
        int start = 0;
        int end = s.length();
        while (start < end && isWhitespace(s.charAt(start)))
            start++;
        while (end > start && isWhitespace(s.charAt(end - 1)))
            end--;
        return s.substring(start, end);
    }

    private static String bytesToString(Bytes bytes)
    {
        byte[] arr = bytes.toByteArray();
        return new String(arr, StandardCharsets.ISO_8859_1);
    }

    private static int popcount(int b)
    {
        return Integer.bitCount(b & 0xff);
    }

    private static class MicrodosDirent
    {
        String filename;
        int ssn;
        int attr;
        List<Sdw> sdws = new ArrayList<>();
        int sectors;
        int lastSectorBytes;
        int loadSectors;
        int loadAddress;
        int startAddress;
        int length;
        String mode;
        Dirent dirent;

        MicrodosDirent(Bytes bytes, BlockDevice blockDevice) throws IOException
        {
            ByteReader br = new ByteReader(bytes);
            Bytes stemBytes = br.read(6);
            Bytes extBytes = br.read(3);
            String stem = trimWhitespace(bytesToString(stemBytes));
            String ext = trimWhitespace(bytesToString(extBytes));
            filename = String.format("%s.%s", stem, ext);

            br.skip(1);
            ssn = br.readBe16() & 0xffff;
            attr = br.read8() & 0xff;

            Bytes rib = blockDevice.getBlock(ssn);
            ByteReader rbr = new ByteReader(rib);
            sectors = 0;
            for (int i = 0; i < 57; i++)
            {
                if (rbr.remaining() < 2)
                    break;
                int w = rbr.readBe16() & 0xffff;
                if ((w & 0x8000) != 0)
                {
                    sectors = w & 0x7fff;
                    break;
                } else
                {
                    if (rbr.eof())
                        break;
                    int b3 = rbr.read8() & 0xff;
                    w = (w << 8) | b3;
                    int start = w & 0xffff;
                    int len = (w >> 16) + 1;
                    sdws.add(new Sdw(start, len));
                }
            }
            rbr.seek(500);
            lastSectorBytes = rbr.readBe16() & 0xffff;
            loadSectors = rbr.readBe16() & 0xffff;
            loadAddress = rbr.readBe16() & 0xffff;
            startAddress = rbr.readBe16() & 0xffff;

            length = sectors * 512;
            mode = "";

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put("microdos.ssn", Integer.toString(ssn));
            attrs.put("microdos.attr", String.format("0x%x", attr));
            attrs.put("microdos.sdw_count", Integer.toString(sdws.size()));
            attrs.put("microdos.total_sectors", Integer.toString(sectors));
            attrs.put("microdos.lastSectorBytes", Integer.toString(lastSectorBytes));
            attrs.put("microdos.loadSectors", Integer.toString(loadSectors));
            attrs.put("microdos.loadAddress", String.format("0x%x", loadAddress));
            attrs.put("microdos.startAddress", String.format("0x%x", startAddress));

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

    private class MicrodosDirectory
    {
        String volumeLabel;
        int usedBlocks;
        List<MicrodosDirent> dirents = new ArrayList<>();

        MicrodosDirectory() throws IOException
        {
            Bytes rootBlock = blockDevice.getBlock(0);
            Bytes catBlock = blockDevice.getBlock(9);
            Bytes directory = blockDevice.getBlocks(1, 8);

            ByteReader rbr = new ByteReader(rootBlock);
            rbr.seek(20);
            Bytes labelBytes = rbr.read(44);
            volumeLabel = trimWhitespace(bytesToString(labelBytes));

            ByteReader dbr = new ByteReader(directory);
            while (!dbr.eof())
            {
                Bytes direntBytes = dbr.read(16);
                int first = direntBytes.getByte(0) & 0xff;
                if (first == 0 || first == 0xff)
                    continue;
                MicrodosDirent de = new MicrodosDirent(direntBytes, blockDevice);
                dirents.add(de);
            }

            usedBlocks = 0;
            ByteReader cbr = new ByteReader(catBlock);
            for (int i = 0; i < TOTAL_BLOCKS / 8; i++)
            {
                int b = cbr.read8() & 0xff;
                usedBlocks += popcount(b);
            }
        }

        MicrodosDirent findFile(String wanted) throws IOException
        {
            for (MicrodosDirent de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }
            throw new NoSuchFileException(wanted);
        }
    }

    public MicrodosFilesystem(MicrodosProto config, BlockDevice blockDevice)
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
        MicrodosDirectory dir = new MicrodosDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, dir.volumeLabel);
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(TOTAL_BLOCKS));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(BLOCK_SIZE));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        MicrodosDirectory dir = new MicrodosDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (MicrodosDirent de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        MicrodosDirectory dir = new MicrodosDirectory();
        String wanted = path.segments().get(0);
        return dir.findFile(wanted).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        MicrodosDirectory dir = new MicrodosDirectory();
        String wanted = path.segments().get(0);
        MicrodosDirent de = dir.findFile(wanted);

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (Sdw sdw : de.sdws)
        {
            Bytes span = blockDevice.getBlocks(sdw.start, sdw.length);
            bw.write(span);
        }
        return data.slice(512);
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
