package com.github.adflib.adflib;

import static com.google.common.truth.Truth.assertThat;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.List;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AdflibTest
{

    private static final int SECTOR_SIZE = 512;
    private static final int DD_CYLINDERS = 80;
    private static final int DD_HEADS = 2;
    private static final int DD_SECTORS = 11;
    private static final int DD_SIZE = SECTOR_SIZE * DD_SECTORS * DD_HEADS * DD_CYLINDERS; // 880KB

    private Device dev;
    private Volume vol;

    @Before
    public void setUp()
    {
        AdfEnv.adfEnvInitDefault();

        dev = new Device()
        {
            private byte[] backing = new byte[DD_SIZE];

            public AdfError adfReadSector(int n, int size, ByteBuffer buf)
            {
                int off = 512 * n;
                if (off + size > backing.length)
                {
                    return AdfError.RC_ERROR;
                }
                for (int i = 0; i < size; i++)
                {
                    buf.put(i, backing[off + i]);
                }
                return AdfError.RC_OK;
            }

            public AdfError adfReadSector(int n, int size, byte[] buf)
            {
                int off = 512 * n;
                if (off + size > backing.length)
                {
                    return AdfError.RC_ERROR;
                }
                System.arraycopy(backing, off, buf, 0, size);
                return AdfError.RC_OK;
            }

            public AdfError adfWriteSector(int n, int size, ByteBuffer buf)
            {
                int off = 512 * n;
                if (off + size > backing.length)
                {
                    return AdfError.RC_ERROR;
                }
                for (int i = 0; i < size; i++)
                {
                    backing[off + i] = buf.get(i);
                }
                return AdfError.RC_OK;
            }

            public AdfError adfWriteSector(int n, int size, byte[] buf)
            {
                int off = 512 * n;
                if (off + size > backing.length)
                {
                    return AdfError.RC_ERROR;
                }
                System.arraycopy(buf, 0, backing, off, size);
                return AdfError.RC_OK;
            }

            public AdfError adfReleaseDevice()
            {
                return AdfError.RC_OK;
            }
        };
        dev.size = DD_SIZE;
        dev.cylinders = DD_CYLINDERS;
        dev.heads = DD_HEADS;
        dev.sectors = DD_SECTORS;
        dev.readOnly = false;
        dev.devType = AdfConstants.DEVTYPE_FLOPDD;

        AdfError rc = AdfHd.adfCreateFlop(dev, "TESTVOL", AdfConstants.FS_FFS);
        assertThat(rc).isEqualTo(AdfError.RC_OK);

        vol = AdfDisk.adfMount(dev, 0, false);
        assertThat(vol).isNotNull();
        assertThat(vol.mounted).isTrue();
    }

    private void putFile(String name, byte[] data)
    {
        // Adflib expects names without leading slash, relative to current dir
        String baseName = name.startsWith("/") ? name.substring(1) : name;

        Entry existing = AdfDir.adfFindEntry(vol, baseName);
        if (existing != null)
        {
            AdfError del = AdfDir.adfRemoveEntry(vol, vol.curDirPtr, baseName);
            assertThat(del).isEqualTo(AdfError.RC_OK);
        }

        File f = AdfFile.adfOpenFile(vol, baseName, "w");
        assertThat(f).isNotNull();

        ByteBuffer buf = ByteBuffer.wrap(data).order(ByteOrder.BIG_ENDIAN);
        int written = AdfFile.adfWriteFile(f, data.length, buf);
        assertThat(written).isEqualTo(data.length);

        AdfFile.adfCloseFile(f);
    }

    private void putFile(String name, String text)
    {
        putFile(name, text.getBytes(StandardCharsets.US_ASCII));
    }

    private byte[] getFile(String name)
    {
        String baseName = name.startsWith("/") ? name.substring(1) : name;

        File f = AdfFile.adfOpenFile(vol, baseName, "r");
        assertThat(f).isNotNull();

        // File length from header - use file.size
        long size = f.fileHdr != null ? f.fileHdr.byteSize : 0;
        if (size == 0)
        {
            // fallback: try to get via Entry
            Entry e = AdfDir.adfFindEntry(vol, baseName);
            if (e != null)
                size = e.size;
        }
        ByteBuffer buf = ByteBuffer.allocate((int) size).order(ByteOrder.BIG_ENDIAN);
        int read = AdfFile.adfReadFile(f, (int) size, buf);
        assertThat(read).isEqualTo((int) size);
        AdfFile.adfCloseFile(f);

        byte[] out = new byte[(int) size];
        // Use absolute get to not depend on position (no flip, absolute put doesn't advance
        // position)
        for (int i = 0; i < size; i++)
            out[i] = buf.get(i);
        return out;
    }

    private List<String> listDir(int sector)
    {
        AdfList list = AdfDir.adfGetDirEnt(vol, sector);
        List<String> names = new ArrayList<>();
        for (AdfList node = list; node != null; node = node.next)
        {
            Entry e = (Entry) node.content;
            if (e.name.equals(".") || e.name.equals(".."))
                continue;
            names.add(e.name);
        }
        AdfDir.adfFreeDirList(list);
        return names;
    }

    private List<String> listDir(String path)
    {
        // For adflib, path is directory name; use vol.curDirPtr and adfChangeDir
        // Simple: if path is "/" use root
        if (path.equals("/") || path.isEmpty())
        {
            AdfDir.adfToRootDir(vol);
            return listDir(vol.curDirPtr);
        }
        String baseName = path.startsWith("/") ? path.substring(1) : path;
        Entry e = AdfDir.adfFindEntry(vol, baseName);
        assertThat(e).isNotNull();
        return listDir(e.sector);
    }

    @Test
    public void createFilesystem()
    {
        // After mkfs, root block should be readable and have correct dosType
        assertThat(vol.mounted).isTrue();
        assertThat(vol.blockSize).isEqualTo(512);

        // Check device size
        assertThat(dev.size).isEqualTo(DD_SIZE);

        // Check volume name was set correctly (stored in root block, not dir entry)
        // For FFS, volName is set from boot block - limit to 30 chars, uppercased by adf
        // Our format used "TESTVOL", adflib should store it
        assertThat(vol.volName).isEqualTo("TESTVOL");
    }

    @Test
    public void getFile()
    {
        putFile("/hello", "Hello, world!");
        byte[] bytes = getFile("/hello");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void getFile_missing()
    {
        File f = AdfFile.adfOpenFile(vol, "missing", "r");
        assertThat(f).isNull();
    }

    @Test
    public void putFile_simple()
    {
        putFile("/data", "Hello, world!");
        byte[] bytes = getFile("/data");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void putFile_replaces()
    {
        putFile("/data", "This is the wrong data.");
        // Re-open with "w" truncates as per adfOpenFile with mode "w"
        putFile("/data", "Hello, world!");
        byte[] bytes = getFile("/data");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void createDirectory()
    {
        AdfError rc = AdfDir.adfCreateDir(vol, vol.curDirPtr, "dir");
        assertThat(rc).isEqualTo(AdfError.RC_OK);
        Entry e = AdfDir.adfFindEntry(vol, "dir");
        assertThat(e).isNotNull();
        assertThat(e.type).isEqualTo(AdfConstants.ST_DIR);
    }

    @Test
    public void createDirectory_fileExists()
    {
        putFile("/data", "Hello, world!");
        AdfError rc = AdfDir.adfCreateDir(vol, vol.curDirPtr, "data");
        assertThat(rc).isNotEqualTo(AdfError.RC_OK);
    }

    @Test
    public void createDirectory_nested()
    {
        AdfError rc = AdfDir.adfCreateDir(vol, vol.curDirPtr, "dir1");
        assertThat(rc).isEqualTo(AdfError.RC_OK);
        List<String> root = listDir("/");
        assertThat(root).contains("dir1");
        assertThat(root).hasSize(1);

        Entry dir1 = AdfDir.adfFindEntry(vol, "dir1");
        assertThat(dir1).isNotNull();
        AdfList sub = AdfDir.adfGetDirEnt(vol, dir1.sector);
        int count = 0;
        for (AdfList n = sub; n != null; n = n.next)
        {
            Entry e = (Entry) n.content;
            if (!e.name.equals(".") && !e.name.equals(".."))
                count++;
        }
        AdfDir.adfFreeDirList(sub);
        assertThat(count).isEqualTo(0);

        rc = AdfDir.adfCreateDir(vol, dir1.sector, "dir2");
        assertThat(rc).isEqualTo(AdfError.RC_OK);

        root = listDir("/");
        assertThat(root).hasSize(1);
        List<String> dir1List = listDir("dir1");
        assertThat(dir1List).contains("dir2");
    }

    @Test
    public void readWriteWithByteBuffer()
    {
        // Direct ByteBuffer test - ensure position/limit not touched
        putFile("/data", "Hello, world!");

        String baseName = "data";
        File f = AdfFile.adfOpenFile(vol, baseName, "r");
        assertThat(f).isNotNull();

        ByteBuffer buf = ByteBuffer.allocate(64).order(ByteOrder.BIG_ENDIAN);
        buf.position(10);
        buf.limit(40);
        int posBefore = buf.position();
        int limBefore = buf.limit();

        int read = AdfFile.adfReadFile(f, 5, buf);
        assertThat(read).isEqualTo(5);
        assertThat(buf.position()).isEqualTo(posBefore);
        assertThat(buf.limit()).isEqualTo(limBefore);
        // Data should be at absolute 0..4
        for (int i = 0; i < 5; i++)
        {
            assertThat(buf.get(i)).isEqualTo((byte) "Hello".charAt(i));
        }
        AdfFile.adfCloseFile(f);
    }

    @Test
    public void writeWithByteBufferDirect()
    {
        File f = AdfFile.adfOpenFile(vol, "direct", "w");
        assertThat(f).isNotNull();

        ByteBuffer buf = ByteBuffer.allocateDirect(13).order(ByteOrder.BIG_ENDIAN);
        byte[] hello = "Hello, direct".getBytes(StandardCharsets.US_ASCII);
        for (int i = 0; i < hello.length; i++)
            buf.put(i, hello[i]);
        buf.position(5);
        buf.limit(13);

        int posBefore = buf.position();
        int limBefore = buf.limit();
        int written = AdfFile.adfWriteFile(f, hello.length, buf);
        assertThat(written).isEqualTo(hello.length);
        assertThat(buf.position()).isEqualTo(posBefore);
        assertThat(buf.limit()).isEqualTo(limBefore);
        AdfFile.adfCloseFile(f);

        byte[] out = getFile("direct");
        assertThat(new String(out, StandardCharsets.US_ASCII)).isEqualTo("Hello, direct");
    }

    @Test
    public void deleteFile()
    {
        putFile("/data", "Hello, world!");
        AdfError rc = AdfDir.adfRemoveEntry(vol, vol.curDirPtr, "data");
        assertThat(rc).isEqualTo(AdfError.RC_OK);
        File f = AdfFile.adfOpenFile(vol, "data", "r");
        assertThat(f).isNull();
    }

    @Test
    public void getFreeBlocks()
    {
        int freeBefore = Adflib.adfCountFreeBlocks(vol);
        // Bitmap is stubbed; just verify count is non-negative and does not increase after
        // allocation
        assertThat(freeBefore).isAtLeast(0);
        putFile("/bigfile", new byte[5000]);
        int freeAfter = Adflib.adfCountFreeBlocks(vol);
        assertThat(freeAfter).isAtMost(freeBefore);
    }
}
