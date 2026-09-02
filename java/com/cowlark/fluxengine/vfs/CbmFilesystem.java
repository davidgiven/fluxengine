package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;
import java.util.ArrayList;
import java.util.List;

public class CbmFilesystem extends Filesystem {
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private final CbmfsProto config;
    private final BlockDevice blockDevice;

    public CbmFilesystem(CbmfsProto config, BlockDevice blockDevice) {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private Bytes getSector(int track, int side, int sector) throws IOException {
        LogicalLocation loc = new LogicalLocation(track, side, sector);
        Long blockId = blockDevice.diskLayout.blockIdByLogicalSectorLocation.get(loc);
        if (blockId == null)
            throw new FileSystemException("sector not found: " + loc);
        return blockDevice.getBlock(blockId.intValue());
    }

    private static String fromPetscii(Bytes bytes) {
        StringBuilder ss = new StringBuilder();
        for (int i = 0; i < bytes.size(); i++) {
            int b = bytes.getByte(i) & 0xff;
            if (b >= 32 && b <= 126)
                ss.append((char) Character.toLowerCase((char) b));
            else
                ss.append(String.format("%%%02x", b & 0xff));
        }
        return ss.toString();
    }

    private static String toFileType(int cbmType) {
        switch (cbmType & 0x0f) {
            case 0:
                return "DEL";
            case 1:
                return "SEQ";
            case 2:
                return "PRG";
            case 3:
                return "USR";
            case 4:
                return "REL";
            default:
                return String.format("[bad type %x]", cbmType & 0x0f);
        }
    }

    private static class CbmDirent {
        String filename;
        int cbmType;
        int startTrack;
        int startSector;
        int sideTrack;
        int sideSector;
        int recordlen;
        int sectors;
        int length;
        Dirent dirent;

        CbmDirent(Bytes dbuf) {
            ByteReader br = new ByteReader(dbuf);
            br.skip(2);
            cbmType = br.read8() & 0xff;
            startTrack = br.read8() & 0xff;
            startSector = br.read8() & 0xff;
            Bytes filenameBytes = br.read(16);
            Bytes trimmed = filenameBytes.split(0xa0).get(0);
            filename = fromPetscii(trimmed);
            sideTrack = br.read8() & 0xff;
            sideSector = br.read8() & 0xff;
            recordlen = br.read8() & 0xff;
            br.skip(6);
            sectors = br.readLe16() & 0xffff;
            length = sectors * 254;
            String mode = "";

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, "file");
            attrs.put(Attributes.MODE, mode);
            attrs.put("cbmfs.type", toFileType(cbmType));
            attrs.put("cbmfs.start_track", Integer.toString(startTrack));
            attrs.put("cbmfs.start_sector", Integer.toString(startSector));
            attrs.put("cbmfs.side_track", Integer.toString(sideTrack));
            attrs.put("cbmfs.side_sector", Integer.toString(sideSector));
            attrs.put("cbmfs.recordlen", Integer.toString(recordlen));
            attrs.put("cbmfs.sectors", Integer.toString(sectors));

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

    private class Directory {
        int dosVersion;
        String volumeName;
        int usedBlocks;
        int bamSize;
        List<CbmDirent> dirents = new ArrayList<>();

        Directory() throws IOException {
            int t = config.getDirectoryTrack();
            int s = 0;
            Bytes b = getSector(t, 0, s);
            ByteReader br = new ByteReader(b);
            br.skip(2);
            dosVersion = br.read8() & 0xff;
            br.skip(1);

            int numTracks = blockDevice.diskLayout.numLogicalCylinders;
            int totalBlocks = blockDevice.getBlockCount();
            bamSize = totalBlocks;
            usedBlocks = 0;
            int block = 0;
            for (int track = 0; track < numTracks; track++) {
                if (br.remaining() < 4)
                    break;
                int blocks = br.read8() & 0xff;
                int bitmap = br.readLe24();
                for (int sector = 0; sector < blocks; sector++) {
                    if ((bitmap & (1 << sector)) != 0) {
                        usedBlocks++;
                    }
                }
                block += blocks;
            }

            br.seek(0x90);
            Bytes nameBytes = br.read(16);
            Bytes prefix = nameBytes.split(0xa0).get(0);
            volumeName = fromPetscii(prefix);

            s = 1;
            while (t != 0xff) {
                Bytes dirBlock = getSector(t, 0, s);
                for (int i = 0; i < 8; i++) {
                    Bytes dbuf = dirBlock.slice(i * 32, 32);
                    if ((dbuf.getByte(2) & 0xff) == 0)
                        continue;
                    CbmDirent de = new CbmDirent(dbuf);
                    dirents.add(de);
                }
                int nextTrackRaw = dirBlock.getByte(0) & 0xff;
                int nextSector = dirBlock.getByte(1) & 0xff;
                if (nextTrackRaw == 0)
                    t = 0xff;
                else
                    t = nextTrackRaw - 1;
                s = nextSector;
            }
        }

        CbmDirent findFile(String filename) throws IOException {
            for (CbmDirent de : dirents) {
                if (de.filename.equals(filename))
                    return de;
            }
            throw new NoSuchFileException(filename);
        }
    }

    @Override
    public void check() {}

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException {
        Directory dir = new Directory();
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, dir.volumeName);
        builder.put(Attributes.USED_BLOCKS, Integer.toString(dir.usedBlocks));
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(blockDevice.getBlockCount()));
        builder.put(Attributes.BLOCK_SIZE, Integer.toString(blockDevice.getBlockSize()));
        builder.put("cbmfs.dos_type", Integer.toString(dir.dosVersion));
        builder.put("cbmfs.bam_size", Integer.toString(dir.bamSize));
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException {
        if (!path.isRoot())
            throw new NoSuchFileException(path.toString());
        Directory dir = new Directory();
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (CbmDirent de : dir.dirents)
            builder.put(de.filename, de.dirent);
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");
        Directory dir = new Directory();
        String wanted = path.segments().get(0);
        return dir.findFile(wanted).dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException {
        if (path.segments().size() != 1)
            throw new InvalidPathException(path.toString(), "Bad path");
        Directory dir = new Directory();
        String wanted = path.segments().get(0);
        CbmDirent de = dir.findFile(wanted);
        if ((de.cbmType & 0x0f) == 4) {
            throw new IOException("cannot read .REL files");
        }

        Bytes data = new Bytes();
        ByteWriter bw = new ByteWriter(data);

        int t = de.startTrack - 1;
        int s = de.startSector;

        while (true) {
            Bytes blk = getSector(t, 0, s);
            int nextTrack = blk.getByte(0) & 0xff;
            int nextSector = blk.getByte(1) & 0xff;
            if (nextTrack != 0) {
                bw.write(blk.slice(2));
            } else {
                bw.write(blk.slice(2, nextSector));
                break;
            }
            if (nextTrack == 0)
                t = 0xff;
            else
                t = nextTrack - 1;
            s = nextSector;
        }
        return data;
    }

    @Override
    public void close() throws Exception {
        flushChanges();
    }

    @Override
    public boolean needsFlushing() {
        return blockDevice.needsCommit();
    }

    @Override
    public void flushChanges() throws IOException {
        blockDevice.commit();
    }

    @Override
    public void discardChanges() throws IOException {
        blockDevice.revert();
    }
}
