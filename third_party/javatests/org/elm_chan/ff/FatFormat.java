package org.elm_chan.ff;

/**
 * Minimal FAT12 super-floppy formatter for tests.
 * Writes a valid BPB + FATs + root directory so that FatFs can mount the volume.
 * Targets a 2880-sector (1.44M) floppy or smaller, with 512-byte sectors.
 */
public final class FatFormat {

    private FatFormat() {}

    public static void format(RamDisk disk) {
        format(disk, "NO NAME");
    }

    public static void format(RamDisk disk, String label) {
        int totalSectors = disk.getSectorCount();
        int bytesPerSec = 512;
        int secPerClus = 1;
        // For very small images use 2 sectors per cluster to keep FAT small
        if (totalSectors <= 720) {
            secPerClus = 2;
        }
        int reserved = 1;
        int numFats = 2;
        int rootEntries = 224;
        int rootSectors = (rootEntries * 32 + bytesPerSec - 1) / bytesPerSec;
        int dataSectors = totalSectors - reserved - numFats * 1 - rootSectors;
        // Compute FAT sectors for FAT12: need (clusters+2)*1.5 bytes
        int clusters = dataSectors / secPerClus;
        int fatSectors = ((clusters + 2) * 3 / 2 + bytesPerSec - 1) / bytesPerSec;
        // Common 1.44M value is 9; for 720 sectors 2*2* etc -> 3
        if (totalSectors == 2880) fatSectors = 9;
        else if (totalSectors == 720) fatSectors = 3;
        else if (fatSectors < 1) fatSectors = 1;

        // Recompute data sectors with actual fatSectors
        dataSectors = totalSectors - reserved - numFats * fatSectors - rootSectors;
        clusters = dataSectors / secPerClus;

        int fatBase = reserved;
        int dirBase = fatBase + numFats * fatSectors;
        // int dataBase = dirBase + rootSectors; // not written here

        byte[] boot = new byte[512];
        boot[0] = (byte) 0xEB;
        boot[1] = (byte) 0x3C;
        boot[2] = (byte) 0x90;
        writeString(boot, 3, "FLUXENGN", 8);
        stWord(boot, 11, bytesPerSec);
        boot[13] = (byte) secPerClus;
        stWord(boot, 14, reserved);
        boot[16] = (byte) numFats;
        stWord(boot, 17, rootEntries);
        stWord(boot, 19, totalSectors < 65535 ? totalSectors : 0);
        boot[21] = (byte) 0xF0;
        stWord(boot, 22, fatSectors);
        stWord(boot, 24, 18); // sec per track
        stWord(boot, 26, 2);  // heads
        stDword(boot, 28, 0); // hidden
        stDword(boot, 32, totalSectors >= 65535 ? totalSectors : 0);
        boot[36] = (byte) 0x00; // drive number
        boot[37] = 0;
        boot[38] = (byte) 0x29;
        stDword(boot, 39, 0x12345678); // vol ID
        writeString(boot, 43, padRight(label != null ? label : "NO NAME", 11), 11);
        writeString(boot, 54, "FAT12   ", 8);
        // boot code zero
        boot[510] = (byte) 0x55;
        boot[511] = (byte) 0xAA;

        disk.diskWrite(0, boot, 1);

        // FATs
        int fatBytes = fatSectors * 512;
        byte[] fat = new byte[fatBytes];
        // FAT12 first entries: media + 0xFFF
        fat[0] = (byte) 0xF0;
        fat[1] = (byte) 0xFF;
        fat[2] = (byte) 0xFF;
        for (int i = 0; i < numFats; i++) {
            disk.diskWrite(fatBase + i * fatSectors, fat, fatSectors);
        }

        // Root directory
        byte[] root = new byte[rootSectors * 512];
        if (label != null && !label.isEmpty()) {
            String padded = padRight(label.toUpperCase(), 11);
            byte[] entry = new byte[32];
            writeString(entry, 0, padded, 11);
            entry[11] = (byte) 0x08; // volume label
            // zero rest
            System.arraycopy(entry, 0, root, 0, 32);
        }
        disk.diskWrite(dirBase, root, rootSectors);

        // Zero data area (optional, for cleanliness)
        byte[] zeroSec = new byte[512];
        int dataBase = dirBase + rootSectors;
        for (int s = dataBase; s < totalSectors; s++) {
            disk.diskWrite(s, zeroSec, 1);
        }
    }

    private static void stWord(byte[] buf, int off, int val) {
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
    }

    private static void stDword(byte[] buf, int off, int val) {
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
        buf[off + 2] = (byte) ((val >> 16) & 0xFF);
        buf[off + 3] = (byte) ((val >> 24) & 0xFF);
    }

    private static void writeString(byte[] buf, int off, String s, int len) {
        for (int i = 0; i < len; i++) {
            byte c = 0x20;
            if (i < s.length()) {
                c = (byte) s.charAt(i);
            }
            buf[off + i] = c;
        }
    }

    private static String padRight(String s, int len) {
        if (s.length() >= len) return s.substring(0, len);
        StringBuilder sb = new StringBuilder(s);
        while (sb.length() < len) sb.append(' ');
        return sb.toString();
    }
}
