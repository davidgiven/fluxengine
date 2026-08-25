package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_CREATE;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_CREATEDIR;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_DELETE;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_GETDIRENT;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_GETFILE;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_LIST;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.Capability.OP_PUTFILE;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.FileType.IS_DIR;
import static com.cowlark.fluxengine.vfs.FileSystemImpl.FileType.IS_FILE;
import static com.google.common.collect.ImmutableMap.toImmutableMap;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.vfs.FileSystemImpl.Dirent.DirentBuilder;
import com.google.common.collect.ImmutableMap;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Streams;
import de.waldheinz.fs.FileSystemFactory;
import de.waldheinz.fs.FsDirectory;
import de.waldheinz.fs.FsDirectoryEntry;
import de.waldheinz.fs.FsFile;
import de.waldheinz.fs.ReadOnlyException;
import de.waldheinz.fs.fat.FatType;
import de.waldheinz.fs.fat.SuperFloppyFormatter;
import lombok.SneakyThrows;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.file.DirectoryNotEmptyException;
import java.nio.file.FileAlreadyExistsException;
import java.nio.file.FileSystemException;
import java.nio.file.NoSuchFileException;
import java.nio.file.NotDirectoryException;
import java.nio.file.Path;

public class FatFileSystemImpl extends FileSystemImpl
{
    private static final ImmutableSet<Capability> CAPABILITIES = ImmutableSet.of(
            OP_CREATE,
            OP_LIST,
            OP_GETFILE,
            OP_PUTFILE,
            OP_GETDIRENT,
            OP_CREATEDIR,
            OP_DELETE);

    private final FatFsProto config;
    private final BlockDevice blockDevice;
    private final de.waldheinz.fs.BlockDevice fatDevice;
    private de.waldheinz.fs.FileSystem fatFilesystem;

    private static class BlockDeviceAdapter implements de.waldheinz.fs.BlockDevice
    {
        private final BlockDevice underlying;

        BlockDeviceAdapter(BlockDevice underlying)
        {
            this.underlying = underlying;
        }

        @Override
        public long getSize() throws IOException
        {
            return underlying.getBlockCount() * underlying.getBlockSize();
        }

        @Override
        public void read(long offset, ByteBuffer byteBuffer) throws IOException
        {
            Bytes bytes = underlying.getBytes((int) offset, byteBuffer.remaining());
            byteBuffer.put(bytes.toByteArray());
        }

        @Override
        public void write(long offset, ByteBuffer byteBuffer)
                throws ReadOnlyException, IOException, IllegalArgumentException
        {
            underlying.putBytes((int) offset, new Bytes(byteBuffer));
        }

        @Override
        public void flush() throws IOException
        {

        }

        @Override
        public int getSectorSize() throws IOException
        {
            return underlying.getBlockSize();
        }

        @Override
        public void close() throws IOException
        {

        }

        @Override
        public boolean isClosed()
        {
            return false;
        }

        @Override
        public boolean isReadOnly()
        {
            return false;
        }
    }

    public FatFileSystemImpl(FatFsProto config, BlockDevice blockDevice)
    {
        super(CAPABILITIES);
        this.config = config;
        this.blockDevice = blockDevice;
        this.fatDevice = new BlockDeviceAdapter(blockDevice);
    }

    @Override
    public void create(boolean quick, String volumeName) throws IOException
    {
        SuperFloppyFormatter
                .get(fatDevice)
                .setFatType(FatType.FAT12)
                .setOemName("fluxengn")
                .setVolumeLabel(volumeName)
                .format();
    }

    @Override
    public ImmutableMap<String, Dirent> list(Path path) throws IOException
    {
        mount();
        FsDirectory dir = findDir(path);

        return Streams
                .stream(dir)
                .filter(de -> !de.getName().equals(".") && !de.getName().equals(".."))
                .collect(toImmutableMap(FsDirectoryEntry::getName, de -> makeDirent(path, de)));
    }

    @Override
    public Bytes getFile(Path path) throws IOException
    {
        mount();

        FsDirectoryEntry de = findExistingFile(path);
        FsFile file = de.getFile();
        ByteBuffer buffer = ByteBuffer.allocate((int) file.getLength());
        file.read(0, buffer);
        buffer.flip();
        return new Bytes(buffer);
    }

    @Override
    public void createDirectory(Path path) throws IOException
    {
        mount();
        FsDirectory dir = findDir(path.getParent());

        try
        {
            dir.addDirectory(path.getFileName().toString());
            dir.flush();
        } catch (IOException e)
        {
            if (e.getMessage().contains("already exists"))
                throw new FileAlreadyExistsException("file already exists");
            throw e;
        }
        fatFilesystem.flush();
    }

    @Override
    public void putFile(Path path, Bytes bytes) throws IOException
    {
        mount();
        FsDirectory dir = findDir(path.getParent());

        String leaf = path.getFileName().toString();
        dir.remove(leaf);
        FsFile file = dir.addFile(leaf).getFile();
        file.write(0, bytes.toByteBuffer());
        file.flush();
        fatFilesystem.flush();
    }

    @Override
    public Dirent getDirent(Path path) throws IOException
    {
        mount();
        FsDirectoryEntry de = findExistingEntry(path);
        return makeDirent(path.getParent(), de);
    }

    @Override
    public void deleteFile(Path path) throws IOException
    {
        mount();
        FsDirectory dir = findDir(path.getParent());

        String leaf = path.getFileName().toString();
        FsDirectoryEntry entry = dir.getEntry(leaf);
        if (entry == null)
            throw new NoSuchFileException("file not found");
        if (entry.isDirectory())
        {
            for (FsDirectoryEntry de : entry.getDirectory())
            {
                String name = de.getName();
                if (!name.equals(".") && !name.equals(".."))
                    throw new DirectoryNotEmptyException("directory not empty");
            }
        }
        dir.remove(leaf);
        dir.flush();
    }

    @SneakyThrows
    private static Dirent makeDirent(Path dir, FsDirectoryEntry de)
    {
        ImmutableMap.Builder<String, String> attrsBuilder = ImmutableMap.builder();
        DirentBuilder direntBuilder = Dirent
                .builder()
                .setFilename(de.getName())
                .setPath(dir.resolve(de.getName()))
                .setMode("");

        attrsBuilder.put(Attributes.FILENAME, de.getName());

        if (de.isFile())
        {
            long length = de.getFile().getLength();
            direntBuilder.setFileType(IS_FILE).setLength((int) length);
            attrsBuilder
                    .put(Attributes.LENGTH, Long.toString(length))
                    .put(Attributes.FILE_TYPE, "file");
        } else
        {
            direntBuilder.setFileType(IS_DIR);
            attrsBuilder.put(Attributes.FILE_TYPE, "dir");
        }

        return direntBuilder.setAttributes(attrsBuilder.build()).build();
    }

    private FsDirectoryEntry findExistingEntry(Path path) throws IOException
    {
        FsDirectory dir = findDir(path.getParent());

        FsDirectoryEntry entry = dir.getEntry(path.getFileName().toString());
        if (entry == null)
            throw new NoSuchFileException("file not found");
        return entry;
    }

    private FsDirectoryEntry findExistingFile(Path path) throws IOException
    {
        FsDirectoryEntry entry = findExistingEntry(path);
        if (!entry.isFile())
            throw new FileSystemException("not a file");
        return entry;
    }

    private FsDirectory findDir(Path path) throws IOException
    {
        FsDirectory dir = fatFilesystem.getRoot();
        if (path != null)
            for (Path s : path)
            {
                FsDirectoryEntry entry = dir.getEntry(s.toString());
                if (entry == null)
                    throw new NoSuchFileException("path element not found");
                if (!entry.isDirectory())
                    throw new NotDirectoryException(String.format(
                            "'%s' is not a directory",
                            entry.getName()));
                dir = entry.getDirectory();
            }
        return dir;
    }

    private void mount() throws IOException
    {
        if (fatFilesystem == null)
            fatFilesystem = FileSystemFactory.create(fatDevice, false);
    }

    @Override
    public void flushChanges() throws IOException
    {
        if (fatFilesystem != null)
            fatFilesystem.flush();
    }
}
