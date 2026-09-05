package org.mars.hfsutils;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import java.nio.charset.StandardCharsets;
import java.util.Arrays;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mars.hfsutils.os.HfsOs;

@RunWith(JUnit4.class)
public class HfsutilsTest
{
    private static final int BLOCKSZ = HfsConstants.HFS_BLOCKSZ;

    /* In-memory HfsOs backed by a byte array (800KB = 1600 sectors). */
    private static class MemOs extends HfsOs
    {
        final byte[] backing;
        long seekPos;

        MemOs(int sizeBytes)
        {
            this.backing = new byte[sizeBytes];
        }

        @Override
        public int open(String path, int mode)
        {
            return 0;
        }

        @Override
        public int close()
        {
            return 0;
        }

        @Override
        public int same(String path)
        {
            return 0;
        }

        @Override
        public long seek(long offset)
        {
            long totalBlocks = backing.length / BLOCKSZ;
            if (offset == -1)
                return totalBlocks;
            if (offset < 0 || offset > totalBlocks)
                return -1;
            seekPos = offset;
            return offset;
        }

        @Override
        public long read(byte[] buf, long len)
        {
            long offset = seekPos * BLOCKSZ;
            long bytesToRead = len * BLOCKSZ;
            long avail = backing.length - offset;
            if (bytesToRead > avail)
                bytesToRead = avail;
            System.arraycopy(backing, (int) offset, buf, 0, (int) bytesToRead);
            seekPos += len;
            return len;
        }

        @Override
        public long write(byte[] buf, long len)
        {
            long offset = seekPos * BLOCKSZ;
            long bytesToWrite = len * BLOCKSZ;
            System.arraycopy(buf, 0, backing, (int) offset, (int) bytesToWrite);
            seekPos += len;
            return len;
        }
    }

    @Before
    public void setUp()
    {
        /* Clear global state between tests */
        Hfs.hfsMounts = null;
    }

    private MemOs makeOs800k()
    {
        /* 800KB = 1600 blocks * 512 bytes. Minimum for v_geometry. */
        return new MemOs(1600 * BLOCKSZ);
    }

    @Test
    public void formatAndMount() throws Exception
    {
        MemOs os = makeOs800k();
        Hfs.hfsFormat(os, null, 0, HfsConstants.HFS_MODE_RDWR, "TEST", 0, null);

        HfsVol vol = Hfs.hfsMount(os, null, 0, HfsConstants.HFS_MODE_RDWR);
        assertThat(vol).isNotNull();

        HfsVolEnt ent = new HfsVolEnt();
        Hfs.hfsVstat(vol, ent);
        byte[] nameBytes = new byte[ent.name.length];
        for (int i = 0; i < ent.name.length; i++)
            nameBytes[i] = (byte) ent.name[i];
        assertThat(new String(nameBytes, 0, 4, StandardCharsets.US_ASCII)).isEqualTo("TEST");

        Hfs.hfsUmount(vol);
    }

    @Test
    public void createAndReadFile() throws Exception
    {
        MemOs os = makeOs800k();
        Hfs.hfsFormat(os, null, 0, HfsConstants.HFS_MODE_RDWR, "VOL", 0, null);

        HfsVol vol = Hfs.hfsMount(os, null, 0, HfsConstants.HFS_MODE_RDWR);
        assertThat(vol).isNotNull();

        /* Create a file */
        HfsFileHandle file = Hfs.hfsCreate(vol, ":test.txt", "TEXT", "TEST");
        assertThat(file).isNotNull();

        /* Write data */
        byte[] data = "Hello, HFS!".getBytes(StandardCharsets.US_ASCII);
        Hfs.hfsWrite(file, data, data.length);
        Hfs.hfsClose(file);

        /* Re-open and read back */
        HfsFileHandle file2 = Hfs.hfsOpen(vol, ":test.txt");
        assertThat(file2).isNotNull();
        byte[] readBuf = new byte[data.length];
        long readLen = Hfs.hfsRead(file2, readBuf, readBuf.length);
        assertThat(readLen).isEqualTo(data.length);
        Hfs.hfsClose(file2);

        assertThat(new String(readBuf)).isEqualTo("Hello, HFS!");

        Hfs.hfsUmount(vol);
    }

    @Test
    public void mkdirAndList() throws Exception
    {
        MemOs os = makeOs800k();
        Hfs.hfsFormat(os, null, 0, HfsConstants.HFS_MODE_RDWR, "VOL", 0, null);

        HfsVol vol = Hfs.hfsMount(os, null, 0, HfsConstants.HFS_MODE_RDWR);
        assertThat(vol).isNotNull();

        Hfs.hfsMkdir(vol, ":mydir");

        HfsDir dir = Hfs.hfsOpendir(vol, ":");
        assertThat(dir).isNotNull();
        HfsDirEnt ent = new HfsDirEnt();
        int found = 0;
        while (true)
        {
            try
            {
                Hfs.hfsReaddir(dir, ent);
                String name = new String(ent.name, 0, strlen(ent.name));
                if (name.equals("mydir"))
                    found++;
            }
            catch (HfsException e)
            {
                break; /* ENOENT = no more entries */
            }
        }
        Hfs.hfsClosedir(dir);

        assertThat(found).isGreaterThan(0);

        Hfs.hfsUmount(vol);
    }

    private static int strlen(char[] s)
    {
        int i = 0;
        while (i < s.length && s[i] != 0)
            i++;
        return i;
    }
}
