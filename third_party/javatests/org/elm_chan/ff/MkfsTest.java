package org.elm_chan.ff;

import static com.google.common.truth.Truth.assertThat;

import java.nio.charset.StandardCharsets;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class MkfsTest {

    private static long fixedTime() {
        int year = 2025 - 1980;
        int mon = 1;
        int day = 1;
        int hour = 0;
        int min = 0;
        int sec = 0;
        int date = (year << 9) | (mon << 5) | day;
        int time = (hour << 11) | (min << 5) | (sec / 2);
        return ((long) date << 16) | time;
    }

    @Test
    public void mkfsDefaultsCreatesMountableFat12() {
        RamDisk disk = new RamDisk(2880);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        FResult r = fs.mkfs(null, null);
        assertThat(r).isEqualTo(FResult.FR_OK);

        // Mount after mkfs
        r = fs.mount();
        assertThat(r).isEqualTo(FResult.FR_OK);

        // File round-trip proves volume is functional
        Fil fp = new Fil();
        r = fs.open(fp, "/HELLO.TXT", FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] data = "Hello mkfs".getBytes(StandardCharsets.US_ASCII);
        int[] bw = new int[1];
        r = fs.write(fp, data, data.length, bw);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(bw[0]).isEqualTo(data.length);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);

        Fil fr = new Fil();
        r = fs.open(fr, "/HELLO.TXT", FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] buf = new byte[data.length];
        int[] br = new int[1];
        r = fs.read(fr, buf, buf.length, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(new String(buf, StandardCharsets.US_ASCII)).isEqualTo("Hello mkfs");
        r = fs.close(fr);
        assertThat(r).isEqualTo(FResult.FR_OK);

        // Raw-byte assertions: MBR at sector 0, VBR at sector 63
        byte[] mbr = new byte[512];
        assertThat(disk.diskRead(0, mbr, 1)).isEqualTo(DResult.RES_OK);
        assertThat(mbr[510] & 0xFF).isEqualTo(0x55);
        assertThat(mbr[511] & 0xFF).isEqualTo(0xAA);
        // PTE system ID at MBR_Table + 0*SZ_PTE + PTE_System (446+4)
        int sys = mbr[446 + 4] & 0xFF;
        assertThat(sys).isAnyOf(0x01, 0x04, 0x06, 0x0C);

        byte[] vbr = new byte[512];
        assertThat(disk.diskRead(63, vbr, 1)).isEqualTo(DResult.RES_OK);
        String oem = new String(vbr, 3, 8, StandardCharsets.US_ASCII);
        assertThat(oem).isEqualTo("MSDOS5.0");
        assertThat(vbr[510] & 0xFF).isEqualTo(0x55);
        assertThat(vbr[511] & 0xFF).isEqualTo(0xAA);

        // getfree sane
        long[] nclst = new long[1];
        r = fs.getfree("/", nclst);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(nclst[0]).isGreaterThan(0L);
    }

    @Test
    public void mkfsSfdPlacesVolumeAtSectorZero() {
        RamDisk disk = new RamDisk(720);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        MkfsParm opt = new MkfsParm(FatFs.FM_ANY | FatFs.FM_SFD, 1, 0, 512, 0);
        FResult r = fs.mkfs(null, opt);
        assertThat(r).isEqualTo(FResult.FR_OK);

        r = fs.mount();
        assertThat(r).isEqualTo(FResult.FR_OK);

        // Sector 0 is directly VBR, not MBR
        byte[] sec0 = new byte[512];
        assertThat(disk.diskRead(0, sec0, 1)).isEqualTo(DResult.RES_OK);
        String oem = new String(sec0, 3, 8, StandardCharsets.US_ASCII);
        assertThat(oem).isEqualTo("MSDOS5.0");
        assertThat(sec0[510] & 0xFF).isEqualTo(0x55);
        assertThat(sec0[511] & 0xFF).isEqualTo(0xAA);
        // Hidden sectors field at 28 should be 0 for SFD
        long hidden = (sec0[28] & 0xFFL) | ((sec0[29] & 0xFFL) << 8) | ((sec0[30] & 0xFFL) << 16) | ((sec0[31] & 0xFFL) << 24);
        assertThat(hidden).isEqualTo(0L);
    }

    @Test
    public void mkfsFat32OnLargeDisk() {
        // ~48 MB => 100k sectors, forces FAT32 with FM_FAT32 or auto
        RamDisk disk = new RamDisk(100_000);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        MkfsParm opt = new MkfsParm(FatFs.FM_FAT32, 1, 0, 0, 0);
        FResult r = fs.mkfs(null, opt);
        assertThat(r).isEqualTo(FResult.FR_OK);

        r = fs.mount();
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fs.fs_type).isEqualTo(FatFs.FS_FAT32);

        // VBR should have FS32 fields: RootClus 2 at offset 44
        byte[] vbr = new byte[512];
        // FAT32 volume still at b_vol 63 (MBR)
        assertThat(disk.diskRead(63, vbr, 1)).isEqualTo(DResult.RES_OK);
        long rootClus = (vbr[44] & 0xFFL) | ((vbr[45] & 0xFFL) << 8) | ((vbr[46] & 0xFFL) << 16) | ((vbr[47] & 0xFFL) << 24);
        assertThat(rootClus).isEqualTo(2L);
        // FSInfo at b_vol+1 and backup at +7
        byte[] fsinfo = new byte[512];
        assertThat(disk.diskRead(64, fsinfo, 1)).isEqualTo(DResult.RES_OK);
        long lead = (fsinfo[0] & 0xFFL) | ((fsinfo[1] & 0xFFL) << 8) | ((fsinfo[2] & 0xFFL) << 16) | ((fsinfo[3] & 0xFFL) << 24);
        assertThat(lead).isEqualTo(0x41615252L);
        assertThat(fsinfo[510] & 0xFF).isEqualTo(0x55);
        assertThat(fsinfo[511] & 0xFF).isEqualTo(0xAA);

        // File operations on FAT32
        Fil fp = new Fil();
        r = fs.open(fp, "/BIG.TXT", FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] payload = new byte[5000];
        for (int i = 0; i < payload.length; i++) payload[i] = (byte) (i & 0xFF);
        int[] bw = new int[1];
        r = fs.write(fp, payload, payload.length, bw);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);

        Fil fr = new Fil();
        r = fs.open(fr, "/BIG.TXT", FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] buf = new byte[payload.length];
        int[] br = new int[1];
        r = fs.read(fr, buf, buf.length, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(buf).isEqualTo(payload);
        r = fs.close(fr);
        assertThat(r).isEqualTo(FResult.FR_OK);
    }

    @Test
    public void mkfsRejectsTooSmallVolume() {
        RamDisk disk = new RamDisk(100); // <128 sectors
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        FResult r = fs.mkfs(null, null);
        assertThat(r).isEqualTo(FResult.FR_MKFS_ABORTED);
    }

    @Test
    public void mkfsRejectsInvalidDrive() {
        RamDisk disk = new RamDisk(2880);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        FResult r = fs.mkfs("1:", null);
        assertThat(r).isEqualTo(FResult.FR_INVALID_DRIVE);
    }

    @Test
    public void mkfsWriteProtectedFails() {
        RamDisk disk = new RamDisk(2880);
        disk.setWriteProtected(true);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        FResult r = fs.mkfs(null, null);
        assertThat(r).isEqualTo(FResult.FR_WRITE_PROTECTED);
    }

    @Test
    public void mkfsDrivePrefixAccepted() {
        RamDisk disk = new RamDisk(2880);
        FatFs fs = new FatFs(disk, MkfsTest::fixedTime);
        MkfsParm opt = MkfsParm.defaultParm();
        FResult r = fs.mkfs("0:", opt);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.mount("0:", 1);
        assertThat(r).isEqualTo(FResult.FR_OK);
        // also accept mount via "0:/"
        Fil fp = new Fil();
        r = fs.open(fp, "0:/TEST.TXT", FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
    }
}
