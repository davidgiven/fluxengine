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
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class Brother120Filesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final BlockDevice blockDevice;

    public Brother120Filesystem(Brother120FsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.blockDevice = blockDevice;
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        BrotherDirectory dir = new BrotherDirectory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
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

        BrotherDirectory dir = new BrotherDirectory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (BrotherDirent de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        BrotherDirectory dir = new BrotherDirectory();
        BrotherDirent de = dir.findFile(path);

        int sector = de.startSector;
        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);
        while (sector != 0 && sector != 0xffff)
        {
            bw.write(blockDevice.getBlock(sector - 1));
            sector = dir.fat.get(sector) & 0xffff;
        }
        return data;
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        BrotherDirectory dir = new BrotherDirectory();
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

    private static class BrotherDirent
    {
        String filename;
        int brotherType;
        int startSector;
        int sectorLength;
        int length;
        Dirent dirent;

        BrotherDirent(Bytes bytes) throws IOException
        {
            ByteReader br = new ByteReader(bytes);
            Bytes filenameBytes = br.read(8);

            StringBuilder rawSb = new StringBuilder();
            for (int i = 0; i < 8; i++)
                rawSb.append((char) (filenameBytes.getByte(i) & 0xff));
            String raw = rawSb.toString();

            String fname = raw;
            for (int i = 0; i < raw.length(); i++)
            {
                char c = raw.charAt(i);
                if (c == ' ')
                {
                    fname = raw.substring(0, i);
                    break;
                }
                if (c < 32 || c > 126)
                    throw new FileSystemException("Invalid filesystem");
            }
            // Also validate that characters up to fname length are printable
            // (already done) and that raw after space is not checked,
            // matching C++ behaviour.

            filename = fname;

            brotherType = br.read8();
            startSector = br.readBe16();
            sectorLength = br.read8();
            length = sectorLength * 256;

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, "");
            attrs.put("brother120.start_sector", Integer.toString(startSector));
            attrs.put("brother120.type", Integer.toString(brotherType));

            dirent = Dirent
                    .builder()
                    .setPath(VfsPath.of("/").resolve(filename))
                    .setFilename(filename)
                    .setLength(length)
                    .setMode("")
                    .setFileType(IS_FILE)
                    .setAttributes(attrs.build())
                    .build();
        }
    }

    private class BrotherDirectory
    {
        int usedSectors = 0;
        List<BrotherDirent> dirents = new ArrayList<>();
        List<Integer> fat = new ArrayList<>();

        BrotherDirectory() throws IOException
        {
            for (int block = 0; block < 8; block++)
            {
                Bytes bytes = blockDevice.getBlock(block);
                for (int d = 0; d < 256 / 16; d++)
                {
                    Bytes buffer = bytes.slice(d * 16, 16);
                    if ((buffer.getByte(0) & 0x80) != 0)
                        continue;

                    BrotherDirent de = new BrotherDirent(buffer);
                    usedSectors += de.sectorLength;
                    dirents.add(de);
                }
            }

            Bytes fatBytes = blockDevice.getBlocks(8, 4);
            ByteReader br = new ByteReader(fatBytes);

            fat.add(0xffff);
            for (int sector = 1; sector != 468; sector++)
                fat.add(br.readBe16() & 0xffff);
        }

        BrotherDirent findFile(VfsPath path) throws IOException
        {
            if (path.segments().size() != 1)
                throw new InvalidPathException(path.toString(), "Bad path");

            String wanted = path.segments().get(0);
            for (BrotherDirent de : dirents)
            {
                if (de.filename.equals(wanted))
                    return de;
            }

            throw new NoSuchFileException(path.toString());
        }
    }
}
