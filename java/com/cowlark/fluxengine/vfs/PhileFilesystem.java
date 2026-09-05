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
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class PhileFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final BlockDevice blockDevice;
    private final int phileBlockSize;

    public PhileFilesystem(PhileProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.blockDevice = blockDevice;
        int bs = 1024;
        if (config != null)
        {
            int v = config.getBlockSize();
            if (v != 0)
                bs = v;
        }
        this.phileBlockSize = bs;
    }

    private static boolean isWhitespace(char c)
    {
        return c == ' ' || c == '\t' || c == '\n' || c == '\r' || c == '\f' || c == '\u000B' ||
                c == '\0';
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

    private int getLogicalSectorSize()
    {
        return blockDevice.diskLayout.layoutByLogicalLocation.get(new CylinderHead(
                0,
                0)).sectorSize;
    }

    private Bytes getPsosBlock(int number, int count) throws IOException
    {
        int sectorSize = getLogicalSectorSize();
        int blockSectors = phileBlockSize / sectorSize;
        if (blockSectors <= 0)
            blockSectors = 1;
        return blockDevice.getBlocks(number * blockSectors, count * blockSectors);
    }

    private Bytes getPsosBlock(int number) throws IOException
    {
        return getPsosBlock(number, 1);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        PhileDirectory dir = new PhileDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, dir.volumeName);
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(dir.totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.totalBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(phileBlockSize));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        PhileDirectory dir = new PhileDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (PhileDirentEntry de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        PhileDirectory dir = new PhileDirectory();
        String wanted = path.segments().get(0);
        return dir.findFile(wanted).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        PhileDirectory dir = new PhileDirectory();
        String wanted = path.segments().get(0);
        PhileDirentEntry de = dir.findFile(wanted);

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        for (Span span : de.spans)
            bw.write(getPsosBlock(span.startBlock, span.blockCount));
        if (data.size() > de.length)
            data.resize(de.length);
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

    private static class Span
    {
        final int startBlock;
        final int blockCount;

        Span(int startBlock, int blockCount)
        {
            this.startBlock = startBlock;
            this.blockCount = blockCount;
        }
    }

    private static class PhileDirentEntry
    {
        final String filename;
        final int fileno;
        final int length;
        final String mode;
        final String ctime;
        final List<Span> spans;
        final Dirent dirent;

        PhileDirentEntry(int fileno, String filename, Bytes filedes)
        {
            this.fileno = fileno;
            this.filename = filename;

            ByteReader br = new ByteReader(filedes);
            br.seek(0x0e);
            this.length = br.readBe32();

            StringBuilder ss = new StringBuilder();
            ss.append('R');
            if ((filedes.getByte(0) & 0xff & 0x40) != 0)
                ss.append('S');
            this.mode = ss.toString();

            int year = new ByteReader(filedes).seek(4).readBe16() & 0xffff;
            int month = filedes.getByte(6) & 0xff;
            int day = (filedes.getByte(7) & 0xff) + 1;
            int hour = filedes.getByte(10) & 0xff & 0x1f;
            int minute = filedes.getByte(8) & 0xff;
            int second = filedes.getByte(11) & 0xff;
            this.ctime = String.format(
                    "%04d-%02d-%02dT%02d:%02d:%02d",
                    year,
                    month,
                    day,
                    hour,
                    minute,
                    second);

            int spansCount = br.readBe16() & 0xffff;
            this.spans = new ArrayList<>();
            for (int i = 0; i < spansCount; i++)
            {
                int start = br.readBe16() & 0xffff;
                int count = br.readBe16() & 0xffff;
                spans.add(new Span(start, count));
            }

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put(Attributes.CTIME, ctime);
            attrs.put("phile.spans", Integer.toString(spansCount));

            this.dirent = Dirent
                    .builder()
                    .setPath(VfsPath.of("/").resolve(filename))
                    .setFilename(filename)
                    .setLength(length)
                    .setMode(mode)
                    .setFileType(IS_FILE)
                    .setAttributes(attrs.build())
                    .build();
        }
    }

    private class PhileDirectory
    {
        String volumeName;
        int totalBlocks;
        List<PhileDirentEntry> dirents = new ArrayList<>();

        PhileDirectory() throws IOException
        {
            Bytes rootBlock = getPsosBlock(2, 1);
            Bytes volBytes = rootBlock.slice(0, 0x0c);
            volumeName = trimWhitespace(bytesToString(volBytes));

            ByteReader rr = new ByteReader(rootBlock);
            rr.seek(0x18);
            totalBlocks = rr.readBe16() & 0xffff;
            rr.seek(0x1c);
            int bitmapBlockNumber = rr.readBe16() & 0xffff;
            rr.seek(0x1e);
            int filedesBlockNumber = rr.readBe16() & 0xffff;
            rr.seek(0x20);
            int filedesEnd = rr.readBe16() & 0xffff;
            int filedesLength = filedesEnd - filedesBlockNumber + 1;

            Bytes directoryBlock = getPsosBlock(3, 1);
            Bytes filedesBlock = getPsosBlock(filedesBlockNumber, filedesLength);

            ByteReader br = new ByteReader(directoryBlock);
            ByteReader fr = new ByteReader(filedesBlock);
            while (!br.eof())
            {
                int fileno = br.readBe16() & 0xffff;
                Bytes nameBytes = br.read(14);
                String name = trimWhitespace(bytesToString(nameBytes));
                if (fileno != 0)
                {
                    fr.seek(fileno * 64);
                    if (fr.remaining() < 64)
                        continue;
                    Bytes filedes = fr.read(64);
                    PhileDirentEntry entry = new PhileDirentEntry(fileno, name, filedes);
                    dirents.add(entry);
                }
            }
        }

        PhileDirentEntry findFile(String wanted) throws IOException
        {
            for (PhileDirentEntry de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }
            throw new NoSuchFileException(wanted);
        }
    }
}
