package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_GETFSDATA;
import static com.cowlark.fluxengine.vfs.Filesystem.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.Filesystem.FileType.IS_FILE;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.FileSystemException;
import java.nio.file.InvalidPathException;
import java.nio.file.NoSuchFileException;

/**
 * ProDOS filesystem, ported from old_cpp_version/lib/vfs/prodos.cc.
 */
public class ProdosFilesystem extends Filesystem {
    private static final ImmutableSet<Capability> CAPABILITIES =
            ImmutableSet.of(OP_GETFSDATA, OP_LIST, OP_GETFILE, OP_GETDIRENT);

    private static final int BLOCK_SECTORS = 2;
    private static final int ROOT_DIRECTORY_BLOCK = 2;

    private static final int STORAGETYPE_VOLUME = 0xf0;
    private static final int STORAGETYPE_DIRBLOCK = 0xe0;
    private static final int STORAGETYPE_SUBDIR = 0xd0;
    private static final int STORAGETYPE_TREE = 0x30;
    private static final int STORAGETYPE_SAPLING = 0x20;
    private static final int STORAGETYPE_SEEDLING = 0x10;

    private final ProdosProto config;
    private final BlockDevice blockDevice;

    private int allocationBitmapLocation;
    private Bits allocationBitmap;

    private class ProdosDirent {
        final String filename;
        final int storageType;
        final int prodosType;
        final int keyBlock;
        final int blocksUsed;
        final int length;
        final long ctime;
        final int version;
        final int minVersion;
        final int access;
        final int auxType;
        final long mtime;
        final FileType fileType;
        final Dirent dirent;

        ProdosDirent(Bytes de, VfsPath parentPath) {
            ByteReader br = new ByteReader(de);
            int rawStorage = br.read8() & 0xff;
            int namelen = rawStorage & 0x0f;
            int stype = rawStorage & 0xf0;
            Bytes nameBytes = br.read(15);
            Bytes sliced = nameBytes.slice(0, namelen);
            this.filename = new String(sliced.toByteArray(), StandardCharsets.ISO_8859_1);
            this.storageType = stype;
            this.prodosType = br.read8() & 0xff;
            this.keyBlock = br.readLe16() & 0xffff;
            this.blocksUsed = br.readLe16() & 0xffff;
            this.length = br.readLe24() & 0xffffff;
            this.ctime = br.readLe32() & 0xffffffffL;
            this.version = br.read8() & 0xff;
            this.minVersion = br.read8() & 0xff;
            this.access = br.read8() & 0xff;
            this.auxType = br.read8() & 0xff;
            this.mtime = br.readLe32() & 0xffffffffL;

            this.fileType = (storageType == STORAGETYPE_SUBDIR) ? IS_DIR : IS_FILE;

            ImmutableMap.Builder<String, String> attrs = ImmutableMap.builder();
            attrs.put(Attributes.FILENAME, filename);
            attrs.put(Attributes.LENGTH, Integer.toString(length));
            attrs.put(Attributes.FILE_TYPE, fileType == IS_DIR ? "dir" : "file");
            attrs.put(Attributes.MODE, "");
            attrs.put("prodos.storage_type", String.format("0x%x", storageType));
            attrs.put("prodos.prodos_type", Integer.toString(prodosType));
            attrs.put("prodos.key_block", Integer.toString(keyBlock));
            attrs.put("prodos.blocks_used", Integer.toString(blocksUsed));
            attrs.put("prodos.version", Integer.toString(version));
            attrs.put("prodos.min_version", Integer.toString(minVersion));
            attrs.put("prodos.access", Integer.toString(access));
            attrs.put("prodos.aux_type", Integer.toString(auxType));

            VfsPath entryPath;
            if (parentPath == null || parentPath.isRoot()) {
                entryPath = VfsPath.of("/").resolve(filename);
            } else {
                entryPath = parentPath.resolve(filename);
            }

            this.dirent =
                    Dirent.builder()
                            .setPath(entryPath)
                            .setFilename(filename)
                            .setLength(length)
                            .setMode("")
                            .setFileType(fileType)
                            .setAttributes(attrs.build())
                            .build();
        }
    }

    private class Directory {
        final VfsPath dirPath;
        final java.util.List<ProdosDirent> dirents = new java.util.ArrayList<>();

        Directory(VfsPath dirPath, int block) throws IOException {
            this.dirPath = dirPath;
            int cur = block;
            while (cur != 0) {
                cur = readDirectoryBlock(cur);
            }
        }

        ProdosDirent find(String filename) throws IOException {
            for (ProdosDirent de : dirents) {
                if (de.filename.equals(filename)) {
                    return de;
                }
            }
            throw new NoSuchFileException(filename);
        }

        private int readDirectoryBlock(int block) throws IOException {
            Bytes bytes = getLogicalBlock(block);
            ByteReader br = new ByteReader(bytes);
            br.seek(2);
            int nextBlock = br.readLe16() & 0xffff;
            int headerByte = br.read8() & 0xff;
            if ((headerByte & 0xf0) >= 0xe0) {
                br.seek(0x2b);
            } else {
                br.seek(4);
            }
            while (br.pos() < 473) {
                Bytes de = br.read(0x27);
                if ((de.getByte(0) & 0xf0) == 0) {
                    continue;
                }
                dirents.add(new ProdosDirent(de, dirPath));
            }
            return nextBlock;
        }
    }

    public ProdosFilesystem(ProdosProto config, BlockDevice blockDevice) {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
    }

    private Bytes getLogicalBlock(int block) throws IOException {
        return blockDevice.getBlocks(block * BLOCK_SECTORS, BLOCK_SECTORS);
    }

    private Bytes getLogicalBlock(int block, int count) throws IOException {
        return blockDevice.getBlocks(block * BLOCK_SECTORS, count * BLOCK_SECTORS);
    }

    private void mount() throws IOException {
        Bytes rootVolume = getLogicalBlock(ROOT_DIRECTORY_BLOCK);
        if ((rootVolume.getByte(4) & 0xf0) != STORAGETYPE_VOLUME) {
            throw new FileSystemException("Invalid filesystem");
        }
        ByteReader br = new ByteReader(rootVolume);
        br.seek(0x27);
        allocationBitmapLocation = br.readLe16() & 0xffff;
        int totalBlocks = br.readLe16() & 0xffff;
        int bitmapBlocks = (totalBlocks + 4095) / 4096;
        if (bitmapBlocks == 0) {
            allocationBitmap = new Bits(0);
        } else {
            Bits full = getLogicalBlock(allocationBitmapLocation, bitmapBlocks).toBits();
            if (full.size() < totalBlocks) {
                // Should not happen, but guard
                allocationBitmap = full;
            } else {
                allocationBitmap = full.subBits(0, totalBlocks);
            }
        }
    }

    private Directory chdir(VfsPath path) throws IOException {
        VfsPath current = VfsPath.root();
        Directory dir = new Directory(current, ROOT_DIRECTORY_BLOCK);
        for (String element : path.segments()) {
            ProdosDirent entry = dir.find(element);
            if (entry.fileType == IS_FILE) {
                throw new InvalidPathException(
                        path.toString(), "tried to use a file like a directory");
            }
            current = current.resolve(element);
            dir = new Directory(current, entry.keyBlock);
        }
        return dir;
    }

    private void readIndexBlock(Bytes indexBlock, ByteWriter bw) throws IOException {
        ByteReader br = new ByteReader(indexBlock);
        while (!br.eof()) {
            int block = br.readLe16() & 0xffff;
            if (block != 0) {
                bw.write(getLogicalBlock(block));
            } else {
                bw.write(new Bytes(512));
            }
        }
    }

    @Override
    public void check() {}

    @Override
    public ImmutableMap<String, String> getFilesystemMetadata() throws IOException {
        mount();
        Bytes block = getLogicalBlock(ROOT_DIRECTORY_BLOCK);
        int flen = block.getByte(4) & 0x0f;
        String volumename = new String(block.slice(5, flen).toByteArray(), StandardCharsets.ISO_8859_1);
        int usedBlocks = 0;
        if (allocationBitmap != null) {
            for (int i = 0; i < allocationBitmap.size(); i++) {
                if (!allocationBitmap.getBit(i)) {
                    usedBlocks++;
                }
            }
        }
        ByteReader br = new ByteReader(block);
        br.seek(0x29);
        int totalBlocks = br.readLe16() & 0xffff;
        ImmutableMap.Builder<String, String> builder = ImmutableMap.builder();
        builder.put(Attributes.VOLUME_NAME, volumename);
        builder.put(Attributes.TOTAL_BLOCKS, Integer.toString(totalBlocks));
        builder.put(Attributes.USED_BLOCKS, Integer.toString(usedBlocks));
        builder.put(Attributes.BLOCK_SIZE, "512");
        return builder.build();
    }

    @Override
    public ImmutableMap<String, Dirent> list(VfsPath path) throws IOException {
        mount();
        Directory dir = chdir(path);
        ImmutableMap.Builder<String, Dirent> builder = ImmutableMap.builder();
        for (ProdosDirent de : dir.dirents) {
            builder.put(de.filename, de.dirent);
        }
        return builder.build();
    }

    @Override
    public Dirent getDirent(VfsPath path) throws IOException {
        mount();
        if (path.isRoot()) {
            throw new NoSuchFileException(path.toString());
        }
        VfsPath parent = path.getParent();
        if (parent == null) {
            parent = VfsPath.root();
        }
        Directory dir = chdir(parent);
        ProdosDirent entry = dir.find(path.getName());
        return entry.dirent;
    }

    @Override
    public Bytes getFile(VfsPath path) throws IOException {
        mount();
        if (path.isRoot()) {
            throw new InvalidPathException(path.toString(), "tried to use a directory like a file");
        }
        VfsPath parent = path.getParent();
        if (parent == null) {
            parent = VfsPath.root();
        }
        Directory dir = chdir(parent);
        ProdosDirent dirent = dir.find(path.getName());

        Bytes bytes;
        switch (dirent.storageType) {
            case STORAGETYPE_SUBDIR:
                throw new InvalidPathException(
                        path.toString(), "tried to use a directory like a file");

            case STORAGETYPE_SEEDLING: {
                bytes = getLogicalBlock(dirent.keyBlock);
                break;
            }

            case STORAGETYPE_SAPLING: {
                Bytes keyBytes = getLogicalBlock(dirent.keyBlock);
                Bytes data = new Bytes();
                ByteWriter bw = new ByteWriter(data);
                readIndexBlock(keyBytes, bw);
                bytes = data;
                break;
            }

            case STORAGETYPE_TREE: {
                Bytes masterKeyBytes = getLogicalBlock(dirent.keyBlock);
                Bytes data = new Bytes();
                ByteWriter bw = new ByteWriter(data);
                ByteReader br = new ByteReader(masterKeyBytes);
                // This always appends 16MB of data, the maximum amount for a tree file,
                // which is wasteful but simple (mirrors C++).
                while (!br.eof()) {
                    int indexBlock = br.readLe16() & 0xffff;
                    if (indexBlock != 0) {
                        readIndexBlock(getLogicalBlock(indexBlock), bw);
                    } else {
                        bw.write(new Bytes(128 * 1024));
                    }
                }
                bytes = data;
                break;
            }

            default:
                throw new IOException(
                        String.format(
                                "storage type 0x%x isn't supported yet", dirent.storageType));
        }

        if (bytes.size() > dirent.length) {
            bytes = bytes.slice(0, dirent.length);
        }
        return bytes;
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
