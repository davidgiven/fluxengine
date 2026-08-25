package org.elm_chan.ff;

import static com.google.common.truth.Truth.assertThat;

import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FatFsTest {

    private RamDisk disk;
    private FatFs fs;

    // Fixed DOS time: 2025-01-01 00:00:00
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

    @Before
    public void setup() {
        disk = new RamDisk(2880);
        FatFormat.format(disk, "TESTVOL");
        fs = new FatFs(disk, FatFsTest::fixedTime);
        FResult r = fs.mount();
        assertThat(r).isEqualTo(FResult.FR_OK);
    }

    private void putFile(String path, byte[] data) {
        Fil fp = new Fil();
        FResult r = fs.open(fp, path, FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS);
        assertThat(r).isEqualTo(FResult.FR_OK);
        IntRef bw = new IntRef();
        r = fs.write(fp, data, data.length, bw);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(bw.value).isEqualTo(data.length);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
    }

    private void putFile(String path, String text) {
        putFile(path, text.getBytes(StandardCharsets.US_ASCII));
    }

    private byte[] getFile(String path) {
        Fil fp = new Fil();
        FResult r = fs.open(fp, path, FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_OK);
        long size = fs.size(fp);
        byte[] buf = new byte[(int) size];
        IntRef br = new IntRef();
        r = fs.read(fp, buf, (int) size, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(br.value).isEqualTo((int) size);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
        return buf;
    }

    private List<String> listDir(String path) {
        Dir dp = new Dir();
        FResult r = fs.opendir(dp, path);
        assertThat(r).isEqualTo(FResult.FR_OK);
        List<String> names = new ArrayList<>();
        while (true) {
            FilInfo fno = new FilInfo();
            r = fs.readdir(dp, fno);
            assertThat(r).isEqualTo(FResult.FR_OK);
            if (fno.fname.isEmpty()) {
                break;
            }
            if (fno.fname.equals(".") || fno.fname.equals("..")) {
                continue;
            }
            names.add(fno.fname);
        }
        r = fs.closedir(dp);
        assertThat(r).isEqualTo(FResult.FR_OK);
        return names;
    }

    private Map<String, FilInfo> listDirInfo(String path) {
        Dir dp = new Dir();
        FResult r = fs.opendir(dp, path);
        assertThat(r).isEqualTo(FResult.FR_OK);
        Map<String, FilInfo> map = new HashMap<>();
        while (true) {
            FilInfo fno = new FilInfo();
            r = fs.readdir(dp, fno);
            assertThat(r).isEqualTo(FResult.FR_OK);
            if (fno.fname.isEmpty()) {
                break;
            }
            if (fno.fname.equals(".") || fno.fname.equals("..")) {
                continue;
            }
            map.put(fno.fname, fno);
        }
        r = fs.closedir(dp);
        assertThat(r).isEqualTo(FResult.FR_OK);
        return map;
    }

    @Test
    public void createFilesystem() {
        // After format+mount, check boot signature and OEM
        byte[] sec0 = new byte[512];
        DResult dr = disk.diskRead(0, sec0, 1);
        assertThat(dr).isEqualTo(DResult.RES_OK);
        String oem = new String(sec0, 3, 8, StandardCharsets.US_ASCII);
        assertThat(oem).isEqualTo("FLUXENGN");
        // FAT signature: first 3 bytes F0 FF FF
        byte[] fat = new byte[512];
        dr = disk.diskRead(1, fat, 1);
        assertThat(dr).isEqualTo(DResult.RES_OK);
        assertThat(fat[0] & 0xFF).isEqualTo(0xF0);
        assertThat(fat[1] & 0xFF).isEqualTo(0xFF);
        assertThat(fat[2] & 0xFF).isEqualTo(0xFF);
        // Volume label entry in root
        byte[] root = new byte[512];
        // root starts at sector 19 for 2880 image (1 + 2*9)
        dr = disk.diskRead(19, root, 1);
        assertThat(dr).isEqualTo(DResult.RES_OK);
        String labelRaw = new String(root, 0, 11, StandardCharsets.US_ASCII);
        assertThat(labelRaw.trim()).isEqualTo("TESTVOL");
        assertThat(root[11] & 0xFF).isEqualTo(0x08);
    }

    @Test
    public void getFile() {
        putFile("/DATA", "Hello, world!");
        byte[] bytes = getFile("/DATA");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void getFile_missing() {
        Fil fp = new Fil();
        FResult r = fs.open(fp, "/DATA", FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_NO_FILE);
    }

    @Test
    public void putFile_simple() {
        putFile("/DATA", "Hello, world!");
        // Verify via raw read of data sector is not needed; check getFile
        byte[] bytes = getFile("/DATA");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void putFile_replaces() {
        putFile("/DATA", "This is the wrong data.");
        putFile("/DATA", "Hello, world!");
        byte[] bytes = getFile("/DATA");
        assertThat(new String(bytes, StandardCharsets.US_ASCII)).isEqualTo("Hello, world!");
    }

    @Test
    public void createDirectory() {
        FResult r = fs.mkdir("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        r = fs.stat("/DIR", fno);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fno.fattrib & FilInfo.AM_DIR).isNotEqualTo(0);
    }

    @Test
    public void createDirectory_fileExists() {
        putFile("/DATA", "Hello, world!");
        FResult r = fs.mkdir("/DATA");
        assertThat(r).isEqualTo(FResult.FR_EXIST);
    }

    @Test
    public void createDirectory_nested() {
        FResult r = fs.mkdir("/DIR1");
        assertThat(r).isEqualTo(FResult.FR_OK);
        List<String> root = listDir("/");
        assertThat(root).hasSize(1);
        assertThat(root).contains("DIR1");

        List<String> dir1 = listDir("/DIR1");
        assertThat(dir1).isEmpty();

        r = fs.mkdir("/DIR1/DIR2");
        assertThat(r).isEqualTo(FResult.FR_OK);

        root = listDir("/");
        assertThat(root).hasSize(1);
        dir1 = listDir("/DIR1");
        assertThat(dir1).hasSize(1);
        assertThat(dir1).contains("DIR2");

        FilInfo fno = new FilInfo();
        r = fs.stat("/DIR1/DIR2", fno);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fno.fname).isEqualTo("DIR2");
        assertThat(fno.fattrib & FilInfo.AM_DIR).isNotEqualTo(0);
    }

    @Test
    public void createDirectory_middleMissing() {
        FResult r = fs.mkdir("/DIR1/DIR2/DIR3/DIR4");
        assertThat(r).isEqualTo(FResult.FR_NO_PATH);
    }

    @Test
    public void delete_file() {
        putFile("/DATA", "Hello, world!");
        FResult r = fs.unlink("/DATA");
        assertThat(r).isEqualTo(FResult.FR_OK);
        List<String> root = listDir("/");
        assertThat(root).isEmpty();
    }

    @Test
    public void delete_dir() {
        FResult r = fs.mkdir("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.unlink("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        List<String> root = listDir("/");
        assertThat(root).isEmpty();
    }

    @Test
    public void delete_missing() {
        FResult r = fs.unlink("/DIR");
        assertThat(r).isEqualTo(FResult.FR_NO_FILE);
    }

    @Test
    public void delete_middle() {
        FResult r = fs.mkdir("/DIR1");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.mkdir("/DIR1/DIR2");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.unlink("/DIR1");
        assertThat(r).isEqualTo(FResult.FR_DENIED);
    }

    @Test
    public void listFiles() {
        putFile("/DATA", "Hello, world!");
        Map<String, FilInfo> files = listDirInfo("/");
        assertThat(files).hasSize(1);
        FilInfo fno = files.get("DATA");
        assertThat(fno).isNotNull();
        assertThat(fno.fsize).isEqualTo(13);
        assertThat(fno.fattrib & FilInfo.AM_DIR).isEqualTo(0);
        assertThat(fno.fname).isEqualTo("DATA");
    }

    @Test
    public void getDirent() {
        putFile("/DATA", "Hello, world!");
        FilInfo fno = new FilInfo();
        FResult r = fs.stat("/DATA", fno);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fno.fname).isEqualTo("DATA");
        assertThat(fno.fsize).isEqualTo(13);
        assertThat(fno.fattrib & FilInfo.AM_DIR).isEqualTo(0);
    }

    @Test
    public void flushActuallyFlushes() {
        FResult r = fs.mkdir("/DIR1");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.mkdir("/DIR1/DIR2");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.mkdir("/DIR1/DIR2/DIR3");
        assertThat(r).isEqualTo(FResult.FR_OK);
        putFile("/DIR1/DIR2/DIR3/DATA", "Hello, world!");

        // Force flush via sync (FatFs sync happens on close; we also ensure mount stays)
        // Create a second FatFs over same disk
        FatFs fs2 = new FatFs(disk, FatFsTest::fixedTime);
        FResult r2 = fs2.mount();
        assertThat(r2).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        r2 = fs2.stat("/DIR1/DIR2/DIR3/DATA", fno);
        assertThat(r2).isEqualTo(FResult.FR_OK);
        assertThat(fno.fname).isEqualTo("DATA");
    }

    @Test
    public void renameFile() {
        putFile("/OLDNAME", "Hello");
        FResult r = fs.rename("/OLDNAME", "/NEWNAME");
        assertThat(r).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        r = fs.stat("/NEWNAME", fno);
        assertThat(r).isEqualTo(FResult.FR_OK);
        FResult r2 = fs.stat("/OLDNAME", fno);
        assertThat(r2).isEqualTo(FResult.FR_NO_FILE);
        byte[] data = getFile("/NEWNAME");
        assertThat(new String(data, StandardCharsets.US_ASCII)).isEqualTo("Hello");
    }

    @Test
    public void lseekAndRead() {
        putFile("/DATA", "Hello, world!");
        Fil fp = new Fil();
        FResult r = fs.open(fp, "/DATA", FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.lseek(fp, 7);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] buf = new byte[5];
        IntRef br = new IntRef();
        r = fs.read(fp, buf, 5, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(br.value).isEqualTo(5);
        assertThat(new String(buf, StandardCharsets.US_ASCII)).isEqualTo("world");
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
    }

    @Test
    public void getfree() {
        LongRef nclst = new LongRef();
        FResult r = fs.getfree("/", nclst);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(nclst.value).isGreaterThan(0L);
        // After creating a file, free clusters should decrease
        long before = nclst.value;
        putFile("/DATA", "Hello, world!");
        r = fs.getfree("/", nclst);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(nclst.value).isLessThan(before);
    }

    @Test
    public void truncate() {
        putFile("/DATA", "Hello, world!XXXX");
        Fil fp = new Fil();
        FResult r = fs.open(fp, "/DATA", FatFs.FA_WRITE | FatFs.FA_READ);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.lseek(fp, 5);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.truncate(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
        byte[] data = getFile("/DATA");
        assertThat(new String(data, StandardCharsets.US_ASCII)).isEqualTo("Hello");
    }

    @Test
    public void openWithCreateNewFailsIfExists() {
        putFile("/DATA", "Hello");
        Fil fp = new Fil();
        FResult r = fs.open(fp, "/DATA", FatFs.FA_WRITE | FatFs.FA_CREATE_NEW);
        assertThat(r).isEqualTo(FResult.FR_EXIST);
    }

    @Test
    public void writeLargeFile() {
        // Write 2000 bytes spanning multiple clusters
        byte[] payload = new byte[2000];
        for (int i = 0; i < payload.length; i++) {
            payload[i] = (byte) (i & 0xFF);
        }
        putFile("/BIGDAT", payload);
        byte[] read = getFile("/BIGDAT");
        assertThat(read).isEqualTo(payload);
    }
}
