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
        int[] bw = new int[1];
        r = fs.write(fp, data, data.length, bw);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(bw[0]).isEqualTo(data.length);
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
        int[] br = new int[1];
        r = fs.read(fp, buf, (int) size, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(br[0]).isEqualTo((int) size);
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
        int[] br = new int[1];
        r = fs.read(fp, buf, 5, br);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(br[0]).isEqualTo(5);
        assertThat(new String(buf, StandardCharsets.US_ASCII)).isEqualTo("world");
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);
    }

    @Test
    public void getfree() {
        long[] nclst = new long[1];
        FResult r = fs.getfree("/", nclst);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(nclst[0]).isGreaterThan(0L);
        // After creating a file, free clusters should decrease
        long before = nclst[0];
        putFile("/DATA", "Hello, world!");
        r = fs.getfree("/", nclst);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(nclst[0]).isLessThan(before);
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

    @Test
    public void lowercasePreservedViaNTres() {
        // Create with lowercase name; SFN is upper but NT flag preserves case on readback
        Fil fp = new Fil();
        FResult r = fs.open(fp, "/lower.txt", FatFs.FA_WRITE | FatFs.FA_CREATE_ALWAYS);
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.close(fp);
        assertThat(r).isEqualTo(FResult.FR_OK);

        // readdir should return the name in the original case
        java.util.List<String> names = listDir("/");
        assertThat(names).contains("lower.txt");

        // stat should find it case-insensitively via create_name upcasing
        FilInfo fno = new FilInfo();
        r = fs.stat("/LOWER.TXT", fno);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fno.fname).isEqualTo("lower.txt");

        // get_fileinfo via stat on the exact lowercase path also works
        FilInfo fno2 = new FilInfo();
        r = fs.stat("/lower.txt", fno2);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(fno2.fname).isEqualTo("lower.txt");
    }

    @Test
    public void mkdirRollbackDoesNotLeakCluster() {
        // Fill root directory until no free entry (FR_DENIED), assert cluster not leaked
        long[] freeBefore = new long[1];
        FResult r = fs.getfree("/", freeBefore);
        assertThat(r).isEqualTo(FResult.FR_OK);

        int created = 0;
        while (true) {
            String name = String.format("/F%05d", created);
            r = fs.mkdir(name);
            if (r == FResult.FR_DENIED) {
                break;
            }
            assertThat(r).isEqualTo(FResult.FR_OK);
            created++;
            if (created > 600) {
                // safety guard - should have hit DENIED by root limit (224 entries) long before
                break;
            }
        }
        assertThat(created).isGreaterThan(0);
        // Must have failed due to directory full, not disk error
        r = fs.mkdir("/FAIL01");
        assertThat(r).isEqualTo(FResult.FR_DENIED);

        long[] freeAfterFail = new long[1];
        r = fs.getfree("/", freeAfterFail);
        assertThat(r).isEqualTo(FResult.FR_OK);
        // Created directories each consumed one cluster; the failing ones should not have consumed any
        assertThat(freeAfterFail[0]).isEqualTo(freeBefore[0] - created);

        // A second failing mkdir must not leak another cluster
        FResult r2 = fs.mkdir("/FAIL02");
        assertThat(r2).isEqualTo(FResult.FR_DENIED);
        long[] freeAfterFail2 = new long[1];
        r = fs.getfree("/", freeAfterFail2);
        assertThat(r).isEqualTo(FResult.FR_OK);
        assertThat(freeAfterFail2[0]).isEqualTo(freeAfterFail[0]);
    }

    @Test
    public void renameFileInRoot() {
        putFile("/OLD.TXT", "content old");
        FResult r = fs.rename("/OLD.TXT", "/NEW.TXT");
        assertThat(r).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        assertThat(fs.stat("/OLD.TXT", fno)).isEqualTo(FResult.FR_NO_FILE);
        assertThat(fs.stat("/NEW.TXT", fno)).isEqualTo(FResult.FR_OK);
        assertThat(new String(getFile("/NEW.TXT"), StandardCharsets.US_ASCII)).isEqualTo("content old");
        assertThat(listDir("/")).contains("NEW.TXT");
    }

    @Test
    public void renameDirectory() {
        FResult r = fs.mkdir("/DIR1");
        assertThat(r).isEqualTo(FResult.FR_OK);
        putFile("/DIR1/FILE.TXT", "inside");
        r = fs.rename("/DIR1", "/DIR2");
        assertThat(r).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        assertThat(fs.stat("/DIR1", fno)).isEqualTo(FResult.FR_NO_FILE);
        assertThat(fs.stat("/DIR2", fno)).isEqualTo(FResult.FR_OK);
        assertThat(fno.fattrib & FilInfo.AM_DIR).isNotEqualTo(0);
        assertThat(new String(getFile("/DIR2/FILE.TXT"), StandardCharsets.US_ASCII)).isEqualTo("inside");
        assertThat(listDir("/")).contains("DIR2");
    }

    @Test
    public void renameMoveFileIntoDirectory() {
        putFile("/FILE.TXT", "hello");
        FResult r = fs.mkdir("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.rename("/FILE.TXT", "/DIR/FILE.TXT");
        assertThat(r).isEqualTo(FResult.FR_OK);
        FilInfo fno = new FilInfo();
        assertThat(fs.stat("/FILE.TXT", fno)).isEqualTo(FResult.FR_NO_FILE);
        assertThat(new String(getFile("/DIR/FILE.TXT"), StandardCharsets.US_ASCII)).isEqualTo("hello");
        assertThat(listDir("/")).contains("DIR");
        assertThat(listDir("/DIR")).contains("FILE.TXT");
    }

    @Test
    public void renameMoveDirectoryIntoItself() {
        FResult r = fs.mkdir("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.mkdir("/DIR/SUB");
        assertThat(r).isEqualTo(FResult.FR_OK);
        // Moving /DIR into its own subtree should fail
        r = fs.rename("/DIR", "/DIR/SUB/MOVED");
        assertThat(r).isEqualTo(FResult.FR_INVALID_NAME);
        // Also moving directly onto itself should fail
        r = fs.rename("/DIR", "/DIR");
        assertThat(r).isEqualTo(FResult.FR_INVALID_NAME);
        // Original still exists intact
        FilInfo fno = new FilInfo();
        assertThat(fs.stat("/DIR", fno)).isEqualTo(FResult.FR_OK);
        assertThat(fs.stat("/DIR/SUB", fno)).isEqualTo(FResult.FR_OK);
    }

    @Test
    public void renameMoveFileOnTopOfAnotherFile() {
        putFile("/A.TXT", "AAA");
        putFile("/B.TXT", "BBB");
        FResult r = fs.rename("/A.TXT", "/B.TXT");
        assertThat(r).isEqualTo(FResult.FR_EXIST);
        // Both files should still exist with original contents
        assertThat(new String(getFile("/A.TXT"), StandardCharsets.US_ASCII)).isEqualTo("AAA");
        assertThat(new String(getFile("/B.TXT"), StandardCharsets.US_ASCII)).isEqualTo("BBB");
    }

    @Test
    public void renameMoveFileOnTopOfDirectory() {
        putFile("/FILE.TXT", "data");
        FResult r = fs.mkdir("/DIR");
        assertThat(r).isEqualTo(FResult.FR_OK);
        r = fs.rename("/FILE.TXT", "/DIR");
        assertThat(r).isEqualTo(FResult.FR_EXIST);
        // Also moving a directory onto a file should fail
        r = fs.rename("/DIR", "/FILE.TXT");
        assertThat(r).isEqualTo(FResult.FR_EXIST);
        assertThat(new String(getFile("/FILE.TXT"), StandardCharsets.US_ASCII)).isEqualTo("data");
        FilInfo fno = new FilInfo();
        assertThat(fs.stat("/DIR", fno)).isEqualTo(FResult.FR_OK);
    }
}
