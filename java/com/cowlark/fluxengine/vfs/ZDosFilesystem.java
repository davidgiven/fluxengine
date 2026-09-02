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
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class ZDosFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private static final int ZDOS_TYPE_DATA = 0x10;
    private static final int ZDOS_TYPE_ASCII = 0x20;
    private static final int ZDOS_TYPE_DIRECTORY = 0x40;
    private static final int ZDOS_TYPE_PROCEDURE = 0x80;

    private static final int ZDOS_MODE_FORCE = 1 << 2;
    private static final int ZDOS_MODE_RANDOM = 1 << 3;
    private static final int ZDOS_MODE_SECRET = 1 << 4;
    private static final int ZDOS_MODE_LOCKED = 1 << 5;
    private static final int ZDOS_MODE_ERASEPROTECT = 1 << 6;
    private static final int ZDOS_MODE_WRITEPROTECT = 1 << 7;

    private final ZDosProto config;
    private final BlockDevice blockDevice;

    public ZDosFilesystem(ZDosProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private static String convertTime(String zdosTime)
    {
        if (zdosTime == null || zdosTime.length() < 6)
            return "";
        String yy = zdosTime.substring(0, 2);
        String mm = zdosTime.substring(2, 4);
        String dd = zdosTime.substring(4, 6);
        try
        {
            int y = Integer.parseInt(yy);
            int m = Integer.parseInt(mm);
            int d = Integer.parseInt(dd);
            int year = (y >= 69) ? 1900 + y : 2000 + y;
            // Validate and format as %FT%T%z with time 00:00:00+0000
            return String.format("%04d-%02d-%02dT00:00:00+0000", year, m, d);
        } catch (NumberFormatException e)
        {
            return zdosTime;
        }
    }

    private static String fileTypeString(int type)
    {
        int masked = type & 0xf0;
        switch (masked)
        {
            case 0:
                return "INVALID";
            case ZDOS_TYPE_DATA:
                return "DATA";
            case ZDOS_TYPE_ASCII:
                return "ASCII";
            case ZDOS_TYPE_DIRECTORY:
                return "DIRECTORY";
            case ZDOS_TYPE_PROCEDURE:
                return "PROCEDURE";
            default:
                return "INVALID";
        }
    }

    private static int toBlockNumber(int sectorId, int track, int sectorsPerTrack)
    {
        return track * sectorsPerTrack + sectorId;
    }

    private static int readBlockNumber(ByteReader br, int sectorsPerTrack)
    {
        int sectorId = br.read8() & 0xff;
        int track = br.read8() & 0xff;
        return toBlockNumber(sectorId, track, sectorsPerTrack);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        ZDosDirectory dir = new ZDosDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(dir.totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, "128");
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        ZDosDirectory dir = new ZDosDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (ZDosDirentEntry de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        ZDosDirectory dir = new ZDosDirectory();
        String wanted = path.segments().get(0);
        return dir.findFile(wanted).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        ZDosDirectory dir = new ZDosDirectory();
        String wanted = path.segments().get(0);
        ZDosDirentEntry de = dir.findFile(wanted);
        de.descriptor.rewind();

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        while (!de.descriptor.eof)
        {
            Bytes rec = de.descriptor.readRecord();
            bw.write(rec);
        }
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

    private class ZDosDescriptor
    {
        private final int sectorsPerTrack;
        int firstRecord;
        int type;
        int recordCount;
        int recordSize;
        int properties;
        int startAddress;
        int lastRecordSize;
        String ctime;
        String mtime;
        int currentRecord;
        boolean eof;

        ZDosDescriptor(int block, int sectorsPerTrack) throws IOException
        {
            this.sectorsPerTrack = sectorsPerTrack;
            Bytes bytes = blockDevice.getBlock(block);
            ByteReader br = new ByteReader(bytes);
            br.seek(8);
            firstRecord = readBlockNumber(br, sectorsPerTrack);
            br.seek(12);
            type = br.read8() & 0xff;
            recordCount = br.readLe16() & 0xffff;
            recordSize = br.readLe16() & 0xffff;
            br.seek(19);
            properties = br.read8() & 0xff;
            startAddress = br.readLe16() & 0xffff;
            lastRecordSize = br.readLe16() & 0xffff;
            br.seek(24);
            Bytes ctimeBytes = br.read(8);
            Bytes mtimeBytes = br.read(8);
            ctime = new String(ctimeBytes.toByteArray(), StandardCharsets.ISO_8859_1);
            mtime = new String(mtimeBytes.toByteArray(), StandardCharsets.ISO_8859_1);

            rewind();
        }

        void rewind()
        {
            currentRecord = firstRecord;
            eof = false;
        }

        Bytes readRecord() throws IOException
        {
            if (eof)
                throw new IllegalStateException("eof");
            int count = recordSize / 0x80;

            Bytes result = new Bytes();
            ByteWriter bw = new ByteWriter(result);

            while (count-- > 0)
            {
                Bytes sector = blockDevice.getBlock(currentRecord);
                ByteReader br = new ByteReader(sector);

                Bytes data = br.read(0x80);
                bw.write(data);
                br.skip(2);
                int sectorId = br.read8() & 0xff;
                int track = br.read8() & 0xff;
                currentRecord = toBlockNumber(sectorId, track, sectorsPerTrack);
                if (sectorId == 0xff)
                    eof = true;
            }

            return result;
        }
    }

    private class ZDosDirentEntry
    {
        String filename;
        int descriptorBlock;
        ZDosDescriptor descriptor;
        int length;
        String mode;
        Dirent dirent;

        ZDosDirentEntry(String filename, int descriptorBlock, int sectorsPerTrack)
                throws IOException
        {
            this.filename = filename;
            this.descriptorBlock = descriptorBlock;
            this.descriptor = new ZDosDescriptor(descriptorBlock, sectorsPerTrack);

            if (descriptor.recordCount == 0)
                length = 0;
            else
                length = (descriptor.recordCount - 1) * descriptor.recordSize +
                        descriptor.lastRecordSize;

            StringBuilder sb = new StringBuilder();
            if ((descriptor.properties & ZDOS_MODE_FORCE) != 0)
                sb.append('F');
            if ((descriptor.properties & ZDOS_MODE_RANDOM) != 0)
                sb.append('R');
            if ((descriptor.properties & ZDOS_MODE_SECRET) != 0)
                sb.append('S');
            if ((descriptor.properties & ZDOS_MODE_LOCKED) != 0)
                sb.append('L');
            if ((descriptor.properties & ZDOS_MODE_ERASEPROTECT) != 0)
                sb.append('E');
            if ((descriptor.properties & ZDOS_MODE_WRITEPROTECT) != 0)
                sb.append('W');
            mode = sb.toString();

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put("zdos.descriptor_record", Integer.toString(descriptorBlock));
            attrs.put("zdos.first_record", Integer.toString(descriptor.firstRecord));
            attrs.put("zdos.record_size", Integer.toString(descriptor.recordSize));
            attrs.put("zdos.record_count", Integer.toString(descriptor.recordCount));
            attrs.put("zdos.last_record_size", Integer.toString(descriptor.lastRecordSize));
            attrs.put("zdos.start_address", String.format("0x%04x", descriptor.startAddress));
            attrs.put("zdos.type", fileTypeString(descriptor.type));
            attrs.put("zdos.ctime", convertTime(descriptor.ctime));
            attrs.put("zdos.mtime", convertTime(descriptor.mtime));

            dirent = Dirent
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

    private class ZDosDirectory
    {
        int sectorsPerTrack;
        int totalBlocks;
        int usedBlocks;
        List<ZDosDirentEntry> dirents = new ArrayList<>();

        ZDosDirectory() throws IOException
        {
            CylinderHead ch = new CylinderHead(0, 0);
            LogicalTrackLayout ltl = blockDevice.diskLayout.layoutByLogicalLocation.get(ch);
            if (ltl == null)
                throw new FileSystemException("Invalid filesystem");
            sectorsPerTrack = ltl.numSectors;

            int track = 0;
            int sector = 0;
            if (config != null && config.hasFilesystemStart())
            {
                track = config.getFilesystemStart().getTrack();
                sector = config.getFilesystemStart().getSector();
            }
            int rootBlock = toBlockNumber(sector, track, sectorsPerTrack);
            ZDosDescriptor zd = new ZDosDescriptor(rootBlock, sectorsPerTrack);
            if (zd.type != ZDOS_TYPE_DIRECTORY)
                throw new FileSystemException("Invalid filesystem");

            totalBlocks = blockDevice.getBlockCount();
            usedBlocks = (zd.recordCount * zd.recordSize) / 0x80 + 1;
            while (!zd.eof)
            {
                Bytes bytes = zd.readRecord();
                ByteReader br = new ByteReader(bytes);
                for (; ; )
                {
                    if (br.eof())
                        break;
                    int len = br.read8() & 0xff;
                    if (len == 0xff)
                        break;
                    int nameLen = len & 0x7f;
                    if (br.remaining() < nameLen)
                        break;
                    Bytes nameBytes = br.read(nameLen);
                    String entryFilename =
                            new String(nameBytes.toByteArray(), StandardCharsets.ISO_8859_1);
                    if (br.remaining() < 2)
                        break;
                    int descriptorBlock = readBlockNumber(br, sectorsPerTrack);

                    ZDosDirentEntry de =
                            new ZDosDirentEntry(entryFilename, descriptorBlock, sectorsPerTrack);
                    usedBlocks += (de.descriptor.recordCount * de.descriptor.recordSize) / 0x80 + 1;
                    dirents.add(de);
                }
            }
        }

        ZDosDirentEntry findFile(String wanted) throws IOException
        {
            for (ZDosDirentEntry de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }
            throw new NoSuchFileException(wanted);
        }
    }
}
