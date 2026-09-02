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
import org.apache.commons.lang3.StringUtils;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class AppleDosFilesystem extends Filesystem
{
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_LIST, OP_GETDIRENT, OP_GETFSDATA, OP_GETFILE);

    private static final int VTOC_BLOCK = 17 * 16;

    private final AppledosProto config;
    private final BlockDevice blockDevice;

    private Bytes _vtoc;
    private List<AppledosEntry> _dirents = new ArrayList<>();

    public AppleDosFilesystem(AppledosProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private Bytes getAppleSector(int number) throws IOException
    {
        int offset = config.hasFilesystemOffsetSectors() ? config.getFilesystemOffsetSectors() : 0;
        return blockDevice.getBlock(number + offset);
    }

    private Bytes getAppleSector(int number, int count) throws IOException
    {
        int offset = config.hasFilesystemOffsetSectors() ? config.getFilesystemOffsetSectors() : 0;
        return blockDevice.getBlocks(number + offset, count);
    }

    private void mount() throws IOException
    {
        _vtoc = getAppleSector(VTOC_BLOCK);
        if ((_vtoc.getByte(0x27) != 122) || (_vtoc.getByte(0x36) != 0) ||
                (_vtoc.getByte(0x37) != 1))
            throw new FileSystemException("Invalid filesystem");

        _dirents.clear();
        int track = _vtoc.getByte(1) & 0xff;
        int sector = _vtoc.getByte(2) & 0xff;
        while (track != 0)
        {
            Bytes dir = getAppleSector(track * 16 + sector);
            ByteReader br = new ByteReader(dir);
            br.seek(0x0b);

            while (!br.eof())
            {
                Bytes fde = br.read(0x23);
                int b0 = fde.getByte(0) & 0xff;
                if ((b0 != 0) && (b0 != 255))
                {
                    ByteReader fbr = new ByteReader(fde);
                    int fTrack = fbr.read8();
                    int fSector = fbr.read8();
                    int fFlags = fbr.read8();
                    Bytes fnBytes = fbr.read(30);
                    int fLength = fbr.readLe16() * 256;

                    StringBuilder sb = new StringBuilder();
                    for (int i = 0; i < fnBytes.size(); i++)
                    {
                        int c = fnBytes.getByte(i) & 0x7f;
                        sb.append((char) c);
                    }
                    String filename = StringUtils.stripEnd(sb.toString(), null);
                    if (filename == null)
                        filename = "";

                    ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
                    attrs.put(Attributes.FILENAME, filename);
                    attrs.put(Attributes.LENGTH, Integer.toString(fLength));
                    attrs.put(Attributes.FILE_TYPE, "file");
                    attrs.put("appledos.flags", String.format("0x%x", fFlags));

                    Dirent dirent = Dirent
                            .builder()
                            .setPath(VfsPath.of("/").resolve(filename))
                            .setFilename(filename)
                            .setLength(fLength)
                            .setMode("")
                            .setFileType(IS_FILE)
                            .setAttributes(attrs.build())
                            .build();

                    AppledosEntry entry = new AppledosEntry();
                    entry.filename = filename;
                    entry.track = fTrack & 0xff;
                    entry.sector = fSector & 0xff;
                    entry.flags = fFlags & 0xff;
                    entry.length = fLength;
                    entry.dirent = dirent;
                    _dirents.add(entry);
                }
            }

            track = dir.getByte(1) & 0xff;
            sector = dir.getByte(2) & 0xff;
        }
    }

    private AppledosEntry find(String filename) throws IOException
    {
        for (AppledosEntry de : _dirents)
        {
            if (de.filename.equals(filename))
                return de;
        }
        throw new NoSuchFileException(filename);
    }

    @Override
    public void check()
    {
    }

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException
    {
        mount();
        int totalBlocks = (_vtoc.getByte(0x34) & 0xff) * (_vtoc.getByte(0x35) & 0xff);
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, "");
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(totalBlocks));
        builder.put(Attributes.USED_BLOCKS, "0");
        builder.put(Attributes.BLOCK_SIZE, "256");
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException
    {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());

        mount();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (AppledosEntry de : _dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        mount();
        AppledosEntry entry = find(path.segments().get(0));
        return entry.dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException
    {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");

        mount();
        AppledosEntry dirent = find(path.segments().get(0));
        int tstrack = dirent.track;
        int tssector = dirent.sector;

        Bytes bytes = new Bytes();
        ByteWriter bw = new ByteWriter(bytes);
        while (tstrack != 0)
        {
            Bytes ts = getAppleSector(tstrack * 16 + tssector);
            ByteReader br = new ByteReader(ts);
            br.seek(0x0c);

            while (!br.eof())
            {
                int track = br.read8();
                int sector = br.read8();
                if (track == 0)
                    return bytes;

                bw.write(getAppleSector(track * 16 + sector));
            }

            tstrack = ts.getByte(1) & 0xff;
            tssector = ts.getByte(2) & 0xff;
        }

        return bytes;
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

    private static class AppledosEntry
    {
        String filename;
        int track;
        int sector;
        int flags;
        int length;
        Dirent dirent;
    }
}
