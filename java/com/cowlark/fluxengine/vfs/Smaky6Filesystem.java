package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class Smaky6Filesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final BlockDevice blockDevice;

    public Smaky6Filesystem(Smaky6FsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.blockDevice = blockDevice;
    }

    @Override
    public void check()
    {
    }

    private int getLogicalSectorSize()
    {
        CylinderHead ch = new CylinderHead(0, 0);
        var ltl = blockDevice.diskLayout.layoutByLogicalLocation.get(ch);
        if (ltl != null)
            return ltl.sectorSize;
        return blockDevice.getBlockSize();
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        SmakyDirectory dir = new SmakyDirectory(VfsPath.root(), 0);
        int usedBlocks = 3;
        for (SmakyEntry de : dir.dirents)
            usedBlocks += (de.endSector - de.startSector);

        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(blockDevice.getBlockCount()));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(getLogicalSectorSize()));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        SmakyDirectory dir = directoryAt(path);
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (SmakyEntry de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        return resolveDirent(path).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        SmakyEntry de = resolveDirent(path);
        if (de.fileType == IS_DIR)
            throw new InvalidPathException(path.toString(), "is a directory");
        Bytes data = blockDevice.getBlocks(de.startSector, de.endSector - de.startSector);
        return data.slice(0, de.length);
    }

    private SmakyDirectory directoryAt(VfsPath path) throws IOException
    {
        if (path.isRoot())
            return new SmakyDirectory(VfsPath.root(), 0);
        if (path.segments().size() == 1)
        {
            SmakyDirectory root = new SmakyDirectory(VfsPath.root(), 0);
            SmakyEntry parent = root.findFile(path.segments().get(0));
            if (parent.fileType != IS_DIR)
                throw new InvalidPathException(path.toString(), "Bad path");
            return new SmakyDirectory(path, parent.startSector);
        }
        throw new InvalidPathException(path.toString(), "Bad path");
    }

    private SmakyEntry resolveDirent(VfsPath path) throws IOException
    {
        if (path.isRoot())
            throw new InvalidPathException(path.toString(), "Bad path");
        VfsPath parentPath = path.getParent();
        String name = path.getName();
        SmakyDirectory dir = directoryAt(parentPath);
        return dir.findFile(name);
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

    private static class SmakyEntry
    {
        String filename;
        int startSector;
        int endSector;
        int lastSectorLength;
        int length;
        FileType fileType;
        Dirent dirent;

        SmakyEntry(Bytes dbuf, VfsPath parentPath)
        {
            StringBuilder ss = new StringBuilder();
            for (int i = 0; i <= 7; i++)
            {
                int c = dbuf.getByte(i) & 0x7f;
                if (c == ' ')
                    break;
                ss.append((char) c);
            }
            for (int i = 8; i <= 9; i++)
            {
                int c = dbuf.getByte(i) & 0x7f;
                if (c == ' ')
                    break;
                if (i == 8)
                    ss.append('.');
                ss.append((char) c);
            }
            filename = ss.toString();

            ByteReader br = new ByteReader(dbuf);
            br.skip(10);
            startSector = br.readLe16() & 0xffff;
            endSector = br.readLe16() & 0xffff;
            int flags = br.readLe16() & 0xffff;
            lastSectorLength = br.readLe16() & 0xffff;
            int loadHi = br.read8() & 0xff;
            int loadLo = br.read8() & 0xff;
            int entryHi = br.read8() & 0xff;
            int entryLo = br.read8() & 0xff;
            int monthBcd = br.read8() & 0xff;
            int yearBcd = br.read8() & 0xff;

            int month = (monthBcd != 0 && monthBcd != 0xff) ? bcdToInt(monthBcd) : 0;
            int year = (yearBcd != 0 && yearBcd != 0xff) ? bcdToInt(yearBcd) : 0;

            int loadAddr = (loadHi << 8) | loadLo;
            int entryAddr = (entryHi << 8) | entryLo;

            if (filename.length() > 3 && filename.substring(filename.length() - 3).equals(".DR"))
                fileType = IS_DIR;
            else
                fileType = IS_FILE;

            if (lastSectorLength != 0)
                length = (endSector - startSector - 1) * 256 + lastSectorLength;
            else
                length = (endSector - startSector) * 256;

            VfsPath path;
            if (parentPath.isRoot())
                path = VfsPath.of("/").resolve(filename);
            else
                path = parentPath.resolve(filename);

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, "");
            attrs.put("smaky6.start_sector", Integer.toString(startSector));
            attrs.put("smaky6.end_sector", Integer.toString(endSector));
            attrs.put("smaky6.sectors", Integer.toString(endSector - startSector));
            attrs.put("smaky6.flags", String.format("0x%04x", flags));
            if (loadAddr != 0)
                attrs.put("smaky6.load_addr", String.format("0x%04x", loadAddr));
            if (entryAddr != 0)
                attrs.put("smaky6.entry_addr", String.format("0x%04x", entryAddr));
            if (month != 0 && year != 0)
            {
                String[] months = {"",
                        "Jan",
                        "Feb",
                        "Mar",
                        "Apr",
                        "May",
                        "Jun",
                        "Jul",
                        "Aug",
                        "Sep",
                        "Oct",
                        "Nov",
                        "Dec"};
                int yearFull = (year >= 78) ? 1900 + year : 2000 + year;
                String monthStr = (month >= 1 && month <= 12) ? months[month] : "?";
                attrs.put("smaky6.date", String.format("%s %d", monthStr, yearFull));
            }

            dirent = Dirent
                    .builder()
                    .setPath(path)
                    .setFilename(filename)
                    .setLength(length)
                    .setMode("")
                    .setFileType(fileType)
                    .setAttributes(attrs.build())
                    .build();
        }

        private static int bcdToInt(int b)
        {
            return (b >> 4) * 10 + (b & 0x0f);
        }
    }

    private class SmakyDirectory
    {
        List<SmakyEntry> dirents = new ArrayList<>();
        VfsPath dirPath;

        SmakyDirectory(VfsPath dirPath, int drStartSector) throws IOException
        {
            this.dirPath = dirPath;
            Bytes bytes = blockDevice.getBlocks(drStartSector, 3);
            parseFrom(bytes, drStartSector);
        }

        private void parseFrom(Bytes bytes, int sectorBase)
        {
            for (int i = 0; i < 32; i++)
            {
                Bytes dbuf = bytes.slice(i * 0x18, 0x18);
                int first = dbuf.getByte(0) & 0xff;
                if (first == 0 || first == 0xff)
                    continue;
                SmakyEntry de = new SmakyEntry(dbuf, dirPath);
                de.startSector += sectorBase;
                de.endSector += sectorBase;
                // rebuild dirent with corrected absolute sectors
                // We need to update attributes that contain sectors
                // Rebuild attributes with corrected values to keep consistency
                // Easiest: patch the dirent's attributes via rebuilding
                ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
                // copy existing but replace sector fields
                for (java.util.Map.Entry<String, String> e : de.dirent.attributes().entrySet())
                {
                    String k = e.getKey();
                    if (k.equals("smaky6.start_sector"))
                        attrs.put(k, Integer.toString(de.startSector));
                    else if (k.equals("smaky6.end_sector"))
                        attrs.put(k, Integer.toString(de.endSector));
                    else if (k.equals("smaky6.sectors"))
                        attrs.put(k, Integer.toString(de.endSector - de.startSector));
                    else
                        attrs.put(k, e.getValue());
                }
                de.dirent = Dirent
                        .builder()
                        .setPath(de.dirent.path())
                        .setFilename(de.dirent.filename())
                        .setLength(de.dirent.length())
                        .setMode(de.dirent.mode())
                        .setFileType(de.dirent.fileType())
                        .setAttributes(attrs.build())
                        .build();
                dirents.add(de);
            }
        }

        SmakyEntry findFile(String wanted) throws IOException
        {
            for (SmakyEntry de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }
            throw new NoSuchFileException(wanted);
        }
    }
}
