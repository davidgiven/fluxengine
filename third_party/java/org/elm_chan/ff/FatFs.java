/*----------------------------------------------------------------------------/
/  FatFs - Generic FAT Filesystem module  R0.16                               /
/-----------------------------------------------------------------------------/
/
/ Copyright (C) 2025, ChaN, all right reserved.
/
/ FatFs module is an open source software. Redistribution and use of FatFs in
/ source and binary forms, with or without modification, are permitted provided
/ that the following condition is met:
/
/ 1. Redistributions of source code must retain the above copyright notice,
/    this condition and the following disclaimer.
/
/ This software is provided by the copyright holder and contributors "AS IS"
/ and any warranties related to this software are DISCLAIMED.
/ The copyright owner or contributors be NOT LIABLE for any damages caused
/ by use of this software.
/
/----------------------------------------------------------------------------*/

package org.elm_chan.ff;

import java.nio.ByteBuffer;
import java.util.Arrays;
import java.util.function.LongSupplier;

/**
 * Java port of ff.c R0.16 (rev 80386) with baked-in configuration:
 * FF_FS_READONLY=0, FF_FS_MINIMIZE=0, FF_USE_FIND=0, FF_USE_MKFS=1,
 * FF_USE_FASTSEEK=0, FF_USE_EXPAND=0, FF_USE_CHMOD=0, FF_USE_LABEL=0,
 * FF_USE_FORWARD=0, FF_USE_STRFUNC=0, FF_CODE_PAGE=932, FF_USE_LFN=0,
 * FF_FS_RPATH=0, FF_VOLUMES=1, FF_MIN_SS=512 FF_MAX_SS=512, FF_LBA64=0,
 * FF_USE_TRIM=0, FF_FS_TINY=0, FF_FS_EXFAT=0, FF_FS_NORTC=0, etc.
 * Single volume, 512-byte sector, no LFN, no exFAT.
 */
public final class FatFs {

    /*--------------------------------------------------------------------------*/
    /* Module Private Definitions - File access mode and open method flags        */
    /* (3rd argument of f_open function)                                         */
    /*--------------------------------------------------------------------------*/
    /* File access mode and open method flags (3rd argument of f_open function) */
    /* Format options (2nd argument of f_mkfs function) - pruned: FF_USE_MKFS == 0 */
    /* Filesystem type (FATFS.fs_type) */
    /* File attribute bits for directory entry (FILINFO.fattrib) */

    //------------------- public constants -------------------
    public static final int FF_MAX_SS = 512;
    public static final int FS_FAT12 = 1;
    public static final int FS_FAT16 = 2;
    public static final int FS_FAT32 = 3;

    public static final int FA_READ = 0x01;
    public static final int FA_WRITE = 0x02;
    public static final int FA_OPEN_EXISTING = 0x00;
    public static final int FA_CREATE_NEW = 0x04;
    public static final int FA_CREATE_ALWAYS = 0x08;
    public static final int FA_OPEN_ALWAYS = 0x10;
    public static final int FA_OPEN_APPEND = 0x30;

    /* Format options (2nd argument of f_mkfs function) */
    public static final int FM_FAT = 0x01 /* FAT volume */;
    public static final int FM_FAT32 = 0x02 /* FAT32 volume */;
    public static final int FM_EXFAT = 0x04 /* exFAT volume - pruned: FF_FS_EXFAT == 0, kept for API completeness */;
    public static final int FM_ANY = 0x07 /* Any of above */;
    public static final int FM_SFD = 0x08 /* Single partition FAT volume */;

    /*--------------------------------------------------------------------------*/
    /* Module Private Definitions                                                    */
    /*--------------------------------------------------------------------------*/

    /* Limits and boundaries */
    /* Character code support macros - IsUpper/IsLower/IsDigit/IsSeparator/IsTerminator/IsSurrogate - inlined as methods IsUpper etc. */
    /* Additional file access control and file status flags for internal use */
    /* Additional file attribute bits for internal use */
    /* Name status flags in fn[11] */
    /* FatFs refers the FAT structures as simple byte array instead of structure member */
    /* because the C structure is not binary compatible between different platforms */

    //------------------- private constants -------------------
    private static final int SZDIRE = 32; /* Size of a directory entry */
    private static final int DDEM = 0xE5; /* Deleted directory entry mark set to DIR_Name[0] */
    private static final int RDDEM = 0x05; /* Replacement of the character collides with DDEM */

    private static final int AM_RDO = 0x01; /* Read only */
    private static final int AM_HID = 0x02; /* Hidden */
    private static final int AM_SYS = 0x04;
    private static final int AM_VOL = 0x08; /* Volume label */
    private static final int AM_LFN = 0x0F; /* LFN entry */
    private static final int AM_DIR = 0x10; /* Directory */
    private static final int AM_ARC = 0x20; /* Archive */
    private static final int AM_MASK = 0x3F; /* Mask of defined bits in FAT */

    private static final int FA_SEEKEND = 0x20; /* Seek to end of the file on file open */
    private static final int FA_MODIFIED = 0x40; /* File has been modified */
    private static final int FA_DIRTY = 0x80; /* FIL.buf[] needs to be written-back */

    private static final int NSFLAG = 11; /* Index of the name status byte */
    private static final int NS_LOSS = 0x01; /* Out of 8.3 format */
    private static final int NS_LFN = 0x02; /* Force to create LFN entry */
    private static final int NS_LAST = 0x04; /* Last segment */
    private static final int NS_BODY = 0x08; /* Lower case flag (body) */
    private static final int NS_EXT = 0x10; /* Lower case flag (ext) */
    private static final int NS_DOT = 0x20; /* Dot entry */
    private static final int NS_NONAME = 0x80; /* Not followed */

    private static final int BS_JmpBoot = 0; /* x86 jump instruction (3-byte) */
    private static final int BS_OEMName = 3; /* OEM name (8-byte) */
    private static final int BPB_BytsPerSec = 11; /* Sector size [byte] (WORD) */
    private static final int BPB_SecPerClus = 13; /* Cluster size [sector] (BYTE) */
    private static final int BPB_RsvdSecCnt = 14; /* Size of reserved area [sector] (WORD) */
    private static final int BPB_NumFATs = 16; /* Number of FATs (BYTE) */
    private static final int BPB_RootEntCnt = 17; /* Size of root directory area for FAT [entry] (WORD) */
    private static final int BPB_TotSec16 = 19; /* Volume size (16-bit) [sector] (WORD) */
    private static final int BPB_Media = 21; /* Media descriptor byte (BYTE) */
    private static final int BPB_FATSz16 = 22; /* FAT size (16-bit) [sector] (WORD) */
    private static final int BPB_SecPerTrk = 24; /* Number of sectors per track for int13h [sector] (WORD) */
    private static final int BPB_NumHeads = 26; /* Number of heads for int13h (WORD) */
    private static final int BPB_HiddSec = 28; /* Volume offset from top of the drive (DWORD) */
    private static final int BPB_TotSec32 = 32; /* Volume size (32-bit) [sector] (DWORD) */
    private static final int BS_DrvNum = 36; /* Physical drive number for int13h (BYTE) */
    private static final int BS_NTres = 37; /* WindowsNT error flag (BYTE) */
    private static final int BS_BootSig = 38; /* Extended boot signature (BYTE) */
    private static final int BS_VolID = 39; /* Volume serial number (DWORD) */
    private static final int BS_VolLab = 43; /* Volume label string (8-byte) */
    private static final int BS_FilSysType = 54; /* Filesystem type string (8-byte) */
    private static final int BS_55AA = 510; /* Boot signature (WORD, for VBR and MBR) */

    private static final int BPB_FATSz32 = 36; /* FAT32: FAT size [sector] (DWORD) */
    private static final int BPB_ExtFlags32 = 40; /* FAT32: Extended flags (WORD) */
    private static final int BPB_FSVer32 = 42; /* FAT32: Filesystem version (WORD) */
    private static final int BPB_RootClus32 = 44; /* FAT32: Root directory cluster (DWORD) */
    private static final int BPB_FSInfo32 = 48; /* FAT32: Offset of FSINFO sector (WORD) */
    private static final int BPB_BkBootSec32 = 50; /* FAT32: Offset of backup boot sector (WORD) */
    private static final int BS_DrvNum32 = 64; /* FAT32: Physical drive number for int13h (BYTE) */
    private static final int BS_NTres32 = 65; /* FAT32: Error flag (BYTE) */
    private static final int BS_BootSig32 = 66; /* FAT32: Extended boot signature (BYTE) */
    private static final int BS_VolID32 = 67; /* FAT32: Volume serial number (DWORD) */
    private static final int BS_VolLab32 = 71; /* FAT32: Volume label string (8-byte) */
    private static final int BS_FilSysType32 = 82; /* FAT32: Filesystem type string (8-byte) */

    private static final int DIR_Name = 0; /* Short file name (11-byte) */
    private static final int DIR_Attr = 11; /* Attribute (BYTE) */
    private static final int DIR_NTres = 12; /* Low case flags of SFN (BYTE) */
    private static final int DIR_CrtTime = 14; /* Created time (DWORD) */
    private static final int DIR_CrtTime10 = 13; /* Created time sub-second (BYTE) */
    private static final int DIR_LstAccDate = 18; /* Last accessed date (WORD) */
    private static final int DIR_FstClusHI = 20; /* Higher 16-bit of first cluster (WORD) */
    private static final int DIR_ModTime = 22; /* Modified time (DWORD) */
    private static final int DIR_FstClusLO = 26; /* Lower 16-bit of first cluster (WORD) */
    private static final int DIR_FileSize = 28; /* File size (DWORD) */

    private static final int FSI_LeadSig = 0; /* FAT32 FSI: Leading signature (DWORD) */
    private static final int FSI_StrucSig = 484; /* FAT32 FSI: Structure signature (DWORD) */
    private static final int FSI_Free_Count = 488; /* FAT32 FSI: Number of free clusters (DWORD) */
    private static final int FSI_Nxt_Free = 492; /* FAT32 FSI: Last allocated cluster (DWORD) */
    private static final int FSI_TrailSig = 508; /* FAT32 FSI: Trailing signature (DWORD) */

    private static final int MBR_Table = 446; /* MBR: Offset of partition table in the MBR */
    private static final int SZ_PTE = 16; /* MBR: Size of a partition table entry */
    private static final int PTE_Boot = 0; /* MBR PTE: Boot indicator */
    private static final int PTE_StHead = 1; /* MBR PTE: Start head in CHS */
    private static final int PTE_StSec = 2; /* MBR PTE: Start sector in CHS */
    private static final int PTE_StCyl = 3; /* MBR PTE: Start cylinder in CHS */
    private static final int PTE_System = 4; /* MBR PTE: System ID */
    private static final int PTE_EdHead = 5; /* MBR PTE: End head in CHS */
    private static final int PTE_EdSec = 6; /* MBR PTE: End sector in CHS */
    private static final int PTE_EdCyl = 7; /* MBR PTE: End cylinder in CHS */
    private static final int PTE_StLba = 8; /* MBR PTE: Start in LBA */
    private static final int PTE_SizLba = 12; /* MBR PTE: Size in LBA */

    private static final int MAX_DIR = 0x200000; /* Max size of FAT directory (byte) */
    private static final int MAX_FAT12 = 0xFF5; /* Max FAT12 clusters (differs from specs, but right for real DOS/Windows behavior) */
    private static final int MAX_FAT16 = 0xFFF5; /* Max FAT16 clusters (differs from specs, but right for real DOS/Windows behavior) */
    private static final int MAX_FAT32 = 0x0FFFFFF5; /* Max FAT32 clusters (not defined in specs, practical limit) */

    private static final int N_SEC_TRACK = 63; /* Sectors per track for determination of drive CHS */
    private static final int[] cst = {1, 4, 16, 64, 256, 512, 0}; /* Cluster size boundary for FAT volume (4K sector unit) */
    private static final int[] cst32 = {1, 2, 4, 8, 16, 32, 0}; /* Cluster size boundary for FAT32 volume (128K sector unit) */

    /* DBCS code range |----- 1st byte -----|  |----------- 2nd byte -----------| */
    /*                  <------>    <------>    <------>    <------>    <------>  */
    // DBCS table for CP932: {0x81,0x9F,0xE0,0xFC,0x40,0x7E,0x80,0xFC,0x00,0x00} /* TBL_DC932 */
    private static final int[] TBL_DC932 = {0x81, 0x9F, 0xE0, 0xFC, 0x40, 0x7E, 0x80, 0xFC, 0x00, 0x00};

    /*--------------------------------------------------------------------------*/
    /* Module Private Work Area                                                    */
    /*--------------------------------------------------------------------------*/
    /* Remark: Variables defined here without initial value shall be guaranteed */
    /*  zero/null at start-up. If not, the linker option or start-up routine is */
    /*  not compliance with C standard. */

    /* Filesystem object structure (FATFS) - pruned for baked config */
    //------------------- FATFS fields (pruned) -------------------
    public int fs_type = 0; /* Filesystem type (0:not mounted) */
    public int n_fats = 0; /* Number of FATs (1 or 2) */
    public int wflag = 0; /* win[] status (b0:dirty) */
    public int fsi_flag = 0; /* Allocation information control (b7:disabled, b0:dirty) */
    public int id = 0; /* Volume mount ID */
    public int n_rootdir = 0; /* Number of root directory entries (FAT12/16) */
    public int csize = 0; /* Cluster size [sectors] */
    public long last_clst = 0; /* Last allocated cluster (invalid if >=n_fatent) */
    public long free_clst = 0; /* Number of free clusters (invalid if >=fs->n_fatent-2) */
    public long n_fatent = 0; /* Number of FAT entries (number of clusters + 2) */
    public long fsize = 0; /* Number of sectors per FAT */
    public long winsect = -1; /* Current sector appearing in the win[] */
    public long volbase = 0; /* Volume base sector */
    public long fatbase = 0; /* FAT base sector */
    public long dirbase = 0; /* Root directory base sector (FAT12/16) or cluster (FAT32/exFAT) */
    public long database = 0; /* Data base sector */
    public final byte[] win = new byte[FF_MAX_SS]; /* Disk access window for directory, FAT (and file data in tiny cfg) */

    private final DiskIo diskIo;
    private final LongSupplier getFatTimeSupplier;
    private static int Fsid = 0;

    /* Filesystem object - constructor initializes FatFs fields as in f_mount */
    //------------------- constructor -------------------
    public FatFs(DiskIo diskIo, LongSupplier get_fattime) {
        if (diskIo == null) {
            throw new IllegalArgumentException("diskIo must not be null");
        }
        this.diskIo = diskIo;
        this.getFatTimeSupplier = get_fattime;
        this.winsect = -1;
    }

    /*--------------------------------------------------------------------------*/
    /* Module Private Functions                                                    */
    /*--------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------*/
    /* Load/Store multi-byte word in the FAT structure                       */
    /*-----------------------------------------------------------------------*/

    //------------------- low level helpers -------------------
    private static int ld_16(byte[] buf, int off) { /* Load a 2-byte little-endian word */
        return (buf[off] & 0xFF) | ((buf[off + 1] & 0xFF) << 8);
    }
    private static long ld_32(byte[] buf, int off) { /* Load a 4-byte little-endian word */
        return (buf[off] & 0xFFL) | ((buf[off + 1] & 0xFFL) << 8) | ((buf[off + 2] & 0xFFL) << 16) | ((buf[off + 3] & 0xFFL) << 24);
    }
    private static void st_16(byte[] buf, int off, int val) { /* Store a 2-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
    }
    private static void st_32(byte[] buf, int off, long val) { /* Store a 4-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
        buf[off + 2] = (byte) ((val >> 16) & 0xFF);
        buf[off + 3] = (byte) ((val >> 24) & 0xFF);
    }
    private static boolean IsUpper(int c) { return c >= 'A' && c <= 'Z'; } /* Character code support macro IsUpper */
    private static boolean IsLower(int c) { return c >= 'a' && c <= 'z'; } /* IsLower */
    private static boolean IsDigit(int c) { return c >= '0' && c <= '9'; } /* IsDigit */
    private static boolean IsSeparator(int c) { return c == '/' || c == '\\'; } /* IsSeparator */
    private static boolean IsTerminator(int c) { return c < '!' ; } /* IsTerminator - LFN disabled so threshold is '!' */

    /* Test if the byte is DBC 1st byte */
    private static boolean dbc_1st(int c) {
        c &= 0xFF;
        if (c >= TBL_DC932[0] && c <= TBL_DC932[1]) return true;
        if (c >= TBL_DC932[2] && c <= TBL_DC932[3]) return true;
        return false;
    }
    /* Test if the byte is DBC 2nd byte */
    private static boolean dbc_2nd(int c) {
        c &= 0xFF;
        if (c >= TBL_DC932[4] && c <= TBL_DC932[5]) return true;
        if (c >= TBL_DC932[6] && c <= TBL_DC932[7]) return true;
        if (TBL_DC932[8] != 0 || TBL_DC932[9] != 0) {
            if (c >= TBL_DC932[8] && c <= TBL_DC932[9]) return true;
        }
        return false;
    }

    /* Timestamp - GET_FATTIME() */
    private long get_fattime() {
        if (getFatTimeSupplier != null) {
            return getFatTimeSupplier.getAsLong() & 0xFFFFFFFFL;
        }
        // Fixed time 2025/01/01 00:00:00
        long t = ((2025 - 1980) << 25) | (1 << 21) | (1 << 16);
        return t & 0xFFFFFFFFL;
    }

    /* Definitions of logical drive to physical location conversion - LD2PD/LD2PT */
    /*-----------------------------------------------------------------------*/
    /* Get logical drive number from path name                               */
    /*-----------------------------------------------------------------------*/
    /* Returns logical drive number (-1:invalid drive number or null pointer) */
    private int get_ldnumber(String[] path) {
        // Mirrors ff.c get_ldnumber for FF_VOLUMES=1, FF_STR_VOLUME_ID=0, FF_FS_RPATH=0
        // Deviation: null path is treated as default drive 0 (Java) instead of -1 (C), to keep public API mount(null) working.
        // path[0] is the input path string; on return path[0] is advanced past drive prefix.
        // Returns 0 for default drive, -1 for invalid.
        if (path == null) return -1;
        if (path[0] == null) {
            path[0] = "";
            return 0;
        }
        String p = path[0];
        // Find colon
        int colonIdx = -1;
        for (int i = 0; i < p.length(); i++) {
            char c = p.charAt(i);
            if (c < '!' || c == ':') {
                if (c == ':') colonIdx = i;
                break;
            }
            if (i+1 >= p.length() || p.charAt(i+1) == ':') {
                // actually need to scan for ':'
            }
        }
        // Simpler: check for DOS style "N:"
        if (p.length() >= 2 && p.charAt(1) == ':') {
            char d = p.charAt(0);
            if (d >= '0' && d <= '9') {
                int vol = d - '0';
                if (vol != 0) return -1;
                path[0] = p.substring(2);
                return vol;
            } else {
                return -1;
            }
        }
        // No drive prefix
        return 0;
    }

    /*-----------------------------------------------------------------------*/
    /* Move/Flush disk access window in the filesystem object                */
    /*-----------------------------------------------------------------------*/
    //------------------- disk window -------------------
    /* Post process on fatal error in the file operations - ABORT is inlined */
    private FResult sync_window() { /* sync_window - Returns FR_OK or FR_DISK_ERR */
        if (wflag != 0) {
            if (diskIo.diskWrite(winsect, win, 1) != DResult.RES_OK) {
                return FResult.FR_DISK_ERR;
            }
            wflag = 0;
            if (winsect - fatbase < fsize) {
                if (n_fats == 2) {
                    diskIo.diskWrite(winsect + fsize, win, 1);
                }
            }
        }
        return FResult.FR_OK;
    }

    private FResult move_window(long sect) { /* move_window - Returns FR_OK or FR_DISK_ERR */
        if (sect != winsect) {
            FResult res = sync_window();
            if (res != FResult.FR_OK) return res;
            if (diskIo.diskRead(sect, win, 1) != DResult.RES_OK) {
                winsect = -1;
                return FResult.FR_DISK_ERR;
            }
            winsect = sect;
        }
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Synchronize filesystem and data on the storage                        */
    /*-----------------------------------------------------------------------*/
    private FResult sync_fs() {
        FResult res = sync_window();
        if (res != FResult.FR_OK) return res;
        if (fsi_flag == 1) {
            fsi_flag = 0;
            if (fs_type == FS_FAT32) {
                // Create FSInfo
                for (int i = 0; i < win.length; i++) win[i] = 0;
                st_32(win, FSI_LeadSig, 0x41615252L);
                st_32(win, FSI_StrucSig, 0x61417272L);
                st_32(win, FSI_Free_Count, free_clst);
                st_32(win, FSI_Nxt_Free, last_clst);
                st_32(win, FSI_TrailSig, 0xAA550000L);
                winsect = volbase + 1;
                diskIo.diskWrite(winsect, win, 1);
            }
        }
        if (diskIo.diskIoctl(DiskIo.CTRL_SYNC, null) != DResult.RES_OK) return FResult.FR_DISK_ERR;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Get physical sector number from cluster number                        */
    /*-----------------------------------------------------------------------*/
    private long clst2sect(long clst) {
        if (clst < 2) return 0;
        clst -= 2;
        if (clst >= n_fatent - 2) return 0;
        return database + (long) csize * clst;
    }

    /*-----------------------------------------------------------------------*/
    /* FAT access - Read value of an FAT entry                               */
    /*-----------------------------------------------------------------------*/
    private long get_fat(long clst) {
        if (clst < 2 || clst >= n_fatent) return 1; // internal error
        long val = 0xFFFFFFFFL;
        FResult res;
        switch (fs_type) {
            case FS_FAT12: {
                int bc = (int) clst + (int)(clst / 2);
                res = move_window(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) break;
                int wc = win[bc % FF_MAX_SS] & 0xFF;
                bc++;
                res = move_window(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) break;
                wc |= (win[bc % FF_MAX_SS] & 0xFF) << 8;
                val = ((clst & 1) != 0) ? (wc >> 4) & 0xFFF : wc & 0xFFF;
                break;
            }
            case FS_FAT16: {
                res = move_window(fatbase + (clst / (FF_MAX_SS / 2)));
                if (res != FResult.FR_OK) break;
                int off = (int) (clst * 2 % FF_MAX_SS);
                val = ld_16(win, off) & 0xFFFFL;
                break;
            }
            case FS_FAT32: {
                res = move_window(fatbase + (clst / (FF_MAX_SS / 4)));
                if (res != FResult.FR_OK) break;
                int off = (int) (clst * 4 % FF_MAX_SS);
                val = ld_32(win, off) & 0x0FFFFFFFL;
                break;
            }
            default: val = 1; break;
        }
        return val;
    }

    /*-----------------------------------------------------------------------*/
    /* FAT access - Change value of an FAT entry                             */
    /*-----------------------------------------------------------------------*/
    private FResult put_fat(long clst, long val) {
        if (clst < 2 || clst >= n_fatent) return FResult.FR_INT_ERR;
        FResult res;
        switch (fs_type) {
            case FS_FAT12: {
                int bc = (int) clst + (int)(clst / 2);
                res = move_window(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) return res;
                int off = bc % FF_MAX_SS;
                int cur = win[off] & 0xFF;
                if ((clst & 1) != 0) {
                    win[off] = (byte) ((cur & 0x0F) | ((val << 4) & 0xF0));
                } else {
                    win[off] = (byte) (val & 0xFF);
                }
                wflag = 1;
                bc++;
                res = move_window(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) return res;
                off = bc % FF_MAX_SS;
                cur = win[off] & 0xFF;
                if ((clst & 1) != 0) {
                    win[off] = (byte) ((val >> 4) & 0xFF);
                } else {
                    win[off] = (byte) ((cur & 0xF0) | ((val >> 8) & 0x0F));
                }
                wflag = 1;
                return FResult.FR_OK;
            }
            case FS_FAT16: {
                res = move_window(fatbase + (clst / (FF_MAX_SS / 2)));
                if (res != FResult.FR_OK) return res;
                int off = (int) (clst * 2 % FF_MAX_SS);
                st_16(win, off, (int) (val & 0xFFFF));
                wflag = 1;
                return FResult.FR_OK;
            }
            case FS_FAT32: {
                res = move_window(fatbase + (clst / (FF_MAX_SS / 4)));
                if (res != FResult.FR_OK) return res;
                int off = (int) (clst * 4 % FF_MAX_SS);
                long cur = ld_32(win, off);
                long newVal = (val & 0x0FFFFFFFL) | (cur & 0xF0000000L);
                st_32(win, off, newVal);
                wflag = 1;
                return FResult.FR_OK;
            }
            default: return FResult.FR_INT_ERR;
        }
    }

    /*-----------------------------------------------------------------------*/
    /* FAT handling - Remove a cluster chain                                 */
    /*-----------------------------------------------------------------------*/
    private FResult remove_chain(long clst, long pclst) {
        if (clst < 2 || clst >= n_fatent) return FResult.FR_INT_ERR;
        FResult res = FResult.FR_OK;
        if (pclst != 0) {
            res = put_fat(pclst, 0xFFFFFFFFL);
            if (res != FResult.FR_OK) return res;
        }
        long nxt;
        long cur = clst;
        do {
            nxt = get_fat(cur);
            if (nxt == 0) break;
            if (nxt == 1) return FResult.FR_INT_ERR;
            if (nxt == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
            res = put_fat(cur, 0);
            if (res != FResult.FR_OK) return res;
            if (free_clst < n_fatent - 2) {
                free_clst++;
                fsi_flag |= 1;
            }
            cur = nxt;
        } while (cur < n_fatent);
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* FAT handling - Stretch a chain or Create a new chain                  */
    /*-----------------------------------------------------------------------*/
    private long create_chain(long clst) {
        long cs, ncl, scl;
        FResult res;
        if (clst == 0) {
            scl = last_clst;
            if (scl == 0 || scl >= n_fatent) scl = 1;
        } else {
            cs = get_fat(clst);
            if (cs < 2) return 1;
            if (cs == 0xFFFFFFFFL) return 0xFFFFFFFFL;
            if (cs < n_fatent) return cs;
            scl = clst;
        }
        if (free_clst == 0) return 0;
        ncl = 0;
        if (scl == clst) {
            ncl = scl + 1;
            if (ncl >= n_fatent) ncl = 2;
            cs = get_fat(ncl);
            if (cs == 1 || cs == 0xFFFFFFFFL) return cs;
            if (cs != 0) {
                cs = last_clst;
                if (cs >= 2 && cs < n_fatent) scl = cs;
                ncl = 0;
            }
        }
        if (ncl == 0) {
            ncl = scl;
            for (;;) {
                ncl++;
                if (ncl >= n_fatent) {
                    ncl = 2;
                    if (ncl > scl) return 0;
                }
                cs = get_fat(ncl);
                if (cs == 0) break;
                if (cs == 1 || cs == 0xFFFFFFFFL) return cs;
                if (ncl == scl) return 0;
            }
        }
        res = put_fat(ncl, 0xFFFFFFFFL);
        if (res == FResult.FR_OK && clst != 0) {
            res = put_fat(clst, ncl);
        }
        if (res == FResult.FR_OK) {
            last_clst = ncl;
            if (free_clst != 0xFFFFFFFFL && free_clst <= n_fatent - 2) {
                free_clst--;
                fsi_flag |= 1;
            }
        } else {
            ncl = (res == FResult.FR_DISK_ERR) ? 0xFFFFFFFFL : 1;
        }
        return ncl;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Fill a cluster with zeros                        */
    /*-----------------------------------------------------------------------*/
    private FResult dir_clear(long clst) {
        // synchronize window
        FResult res = sync_window();
        if (res != FResult.FR_OK) return FResult.FR_DISK_ERR;
        long sect = clst2sect(clst);
        if (sect == 0) return FResult.FR_INT_ERR;
        winsect = sect;
        for (int i = 0; i < win.length; i++) win[i] = 0;
        // Fill cluster with zeros
        for (int n = 0; n < csize; n++) {
            if (diskIo.diskWrite(sect + n, win, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR;
        }
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Set directory index                              */
    /*-----------------------------------------------------------------------*/
    private FResult dir_sdi(Dir dp, long ofs) {
        if (ofs >= MAX_DIR || ofs % SZDIRE != 0) return FResult.FR_INT_ERR;
        dp.dptr = ofs;
        long clst = dp.sclust;
        if (clst == 0 && fs_type == FS_FAT32) {
            clst = dirbase;
        }
        if (clst == 0) {
            if (ofs / SZDIRE >= n_rootdir) return FResult.FR_INT_ERR;
            dp.sect = dirbase;
        } else {
            long csz = (long) csize * FF_MAX_SS;
            long offset = ofs;
            while (offset >= csz) {
                long nxt = get_fat(clst);
                if (nxt == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                if (nxt < 2 || nxt >= n_fatent) return FResult.FR_INT_ERR;
                offset -= csz;
                clst = nxt;
            }
            long s = clst2sect(clst);
            if (s == 0) return FResult.FR_INT_ERR;
            dp.sect = s + offset / FF_MAX_SS;
        }
        dp.clust = clst;
        if (dp.sect == 0) return FResult.FR_INT_ERR;
        dp.dir_ptr = (int) (ofs % FF_MAX_SS);
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Move directory table index next                  */
    /*-----------------------------------------------------------------------*/
    private FResult dir_next(Dir dp, boolean stretch) {
        long ofs = dp.dptr + SZDIRE;
        if (ofs >= MAX_DIR) { dp.sect = 0; return FResult.FR_NO_FILE; }
        if (dp.sect == 0) return FResult.FR_NO_FILE;
        if (ofs % FF_MAX_SS == 0) {
            dp.sect++;
            if (dp.clust == 0) {
                if (ofs / SZDIRE >= n_rootdir) { dp.sect = 0; return FResult.FR_NO_FILE; }
            } else {
                if ((ofs / FF_MAX_SS & (csize - 1)) == 0) {
                    long clst = get_fat(dp.clust);
                    if (clst <= 1) return FResult.FR_INT_ERR;
                    if (clst == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                    if (clst >= n_fatent) {
                        if (!stretch) { dp.sect = 0; return FResult.FR_NO_FILE; }
                        long ncl = create_chain(dp.clust);
                        if (ncl == 0) return FResult.FR_DENIED;
                        if (ncl == 1) return FResult.FR_INT_ERR;
                        if (ncl == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                        if (dir_clear(ncl) != FResult.FR_OK) return FResult.FR_DISK_ERR;
                        clst = ncl;
                    }
                    dp.clust = clst;
                    long sect = clst2sect(clst);
                    if (sect == 0) return FResult.FR_INT_ERR;
                    dp.sect = sect;
                }
            }
        }
        dp.dptr = ofs;
        dp.dir_ptr = (int) (ofs % FF_MAX_SS);
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Reserve a block of directory entries             */
    /*-----------------------------------------------------------------------*/
    private FResult dir_alloc(Dir dp, int nEnt) {
        FResult res = dir_sdi(dp, 0);
        if (res != FResult.FR_OK) return res;
        int n = 0;
        do {
            res = move_window(dp.sect);
            if (res != FResult.FR_OK) break;
            int name = win[dp.dir_ptr] & 0xFF;
            if (name == DDEM || name == 0) {
                n++;
                if (n == nEnt) break;
            } else {
                n = 0;
            }
            res = dir_next(dp, true);
        } while (res == FResult.FR_OK);
        if (res == FResult.FR_NO_FILE) res = FResult.FR_DENIED;
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* Register an object to the directory                                   */
    /*-----------------------------------------------------------------------*/
    /* FR_OK:succeeded, FR_DENIED:no free entry or too many SFN collision, FR_DISK_ERR:disk error */
    private FResult dir_register(Dir dp) {
        FResult res;
        // Non LFN configuration
        res = dir_alloc(dp, 1);        /* Allocate an entry for SFN */
        /* Set SFN entry */
        if (res == FResult.FR_OK) {
            res = move_window(dp.sect);
            if (res == FResult.FR_OK) {
                for (int i = 0; i < SZDIRE; i++) win[dp.dir_ptr + i] = 0;    /* Clean the entry */
                System.arraycopy(dp.fn, 0, win, dp.dir_ptr + DIR_Name, 11);    /* Put SFN */
                win[dp.dir_ptr + DIR_NTres] = (byte) (dp.fn[NSFLAG] & (NS_BODY | NS_EXT));    /* Put low-case flags */
                wflag = 1;
            }
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* Remove an object from the directory                                   */
    /*-----------------------------------------------------------------------*/
    private FResult dir_remove(Dir dp) {
        FResult res;
        res = move_window(dp.sect);
        if (res == FResult.FR_OK) {
            win[dp.dir_ptr + DIR_Name] = (byte) DDEM;    /* Mark the entry 'deleted'.*/
            wflag = 1;
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* Read an object from the directory                                     */
    /*-----------------------------------------------------------------------*/
    private FResult dir_read(Dir dp, int vol) {
        FResult res = FResult.FR_NO_FILE;
        int attr;
        byte et;
        while (dp.sect != 0) {
            res = move_window(dp.sect);
            if (res != FResult.FR_OK) break;
            et = win[dp.dir_ptr];
            if ((et & 0xFF) == 0) {
                res = FResult.FR_NO_FILE; break;
            }
            attr = win[dp.dir_ptr + DIR_Attr] & 0xFF;
            attr &= AM_MASK;
            if ((et & 0xFF) != DDEM && (et & 0xFF) != '.' && attr != AM_LFN && ((attr & ~AM_ARC) == AM_VOL ? 1 : 0) == vol) {
                break;
            }
            res = dir_next(dp, false);
            if (res != FResult.FR_OK) break;
        }
        if (res != FResult.FR_OK) dp.sect = 0;
        return res;
    }

        /*-----------------------------------------------------------------------*/
    /* FAT: Directory handling - Load/Store start cluster number             */
    /*-----------------------------------------------------------------------*/
    private long ldClust(byte[] dir, int off) {
        long cl = ld_16(dir, off + DIR_FstClusLO) & 0xFFFFL;
        if (fs_type == FS_FAT32) {
            cl |= ((long) ld_16(dir, off + DIR_FstClusHI) & 0xFFFFL) << 16;
        }
        return cl;
    }
    private long ldClustFromWin(int ptr) {
        long cl = ld_16(win, ptr + DIR_FstClusLO) & 0xFFFFL;
        if (fs_type == FS_FAT32) cl |= ((long) ld_16(win, ptr + DIR_FstClusHI) & 0xFFFFL) << 16;
        return cl;
    }
    private void stClust(byte[] dir, int off, long cl) {
        st_16(dir, off + DIR_FstClusLO, (int) (cl & 0xFFFF));
        if (fs_type == FS_FAT32) st_16(dir, off + DIR_FstClusHI, (int) ((cl >> 16) & 0xFFFF));
    }
    private void stClustWin(int ptr, long cl) {
        st_16(win, ptr + DIR_FstClusLO, (int) (cl & 0xFFFF));
        if (fs_type == FS_FAT32) st_16(win, ptr + DIR_FstClusHI, (int) ((cl >> 16) & 0xFFFF));
    }

    /* Directory handling - Get file information */
    /*-----------------------------------------------------------------------*/
    /* Get file information from directory entry                             */
    /*-----------------------------------------------------------------------*/
    private void get_fileinfo(Dir dp, FilInfo fno) {
        int si = 0, di = 0;
        if (dp.sect == 0) {
            fno.fname = "";
            return;
        }
        byte[] dir = win;
        int ptr = dp.dir_ptr;
        // Non-LFN configuration with NT flag handling
        StringBuilder sb = new StringBuilder();
        int nt = dir[ptr + DIR_NTres] & 0xFF;
        // Body 0..7
        for (si = 0; si < 8; si++) {
            int c = dir[ptr + si] & 0xFF;
            if (c == ' ') break;
            if (c == RDDEM) c = DDEM;
            // Lower case handling
            if ((nt & NS_BODY) != 0 && c >= 'A' && c <= 'Z') c += 0x20;
            sb.append((char) c);
        }
        // Extension 8..10
        boolean hasExt = false;
        for (int k = 8; k < 11; k++) if ((dir[ptr + k] & 0xFF) != ' ') hasExt = true;
        if (hasExt) {
            sb.append('.');
            for (si = 8; si < 11; si++) {
                int c = dir[ptr + si] & 0xFF;
                if (c == ' ') continue;
                if ((nt & NS_EXT) != 0 && c >= 'A' && c <= 'Z') c += 0x20;
                sb.append((char) c);
            }
        }
        fno.fname = sb.toString();
        fno.fattrib = dir[ptr + DIR_Attr] & AM_MASK;
        fno.fsize = ld_32(dir, ptr + DIR_FileSize);
        fno.ftime = ld_16(dir, ptr + DIR_ModTime) & 0xFFFF;
        fno.fdate = ld_16(dir, ptr + DIR_ModTime + 2) & 0xFFFF;
    }

    private static int compareFilenames(byte[] win, int ptr, byte[] fn) {
        for (int i = 0; i < 11; i++) {
            int a = win[ptr + i] & 0xFF;
            int b = fn[i] & 0xFF;
            if (a != b) return a - b;
        }
        return 0;
    }

    /* Directory handling - Find directory entry */
    private FResult dir_find(Dir dp) {
        FResult res = dir_sdi(dp, 0);
        if (res != FResult.FR_OK) return res;
        do {
            res = move_window(dp.sect);
            if (res != FResult.FR_OK) break;
            int et = win[dp.dir_ptr] & 0xFF;
            if (et == 0) { res = FResult.FR_NO_FILE; break; }
            int attr = win[dp.dir_ptr + DIR_Attr] & 0xFF;
            // In non-LFN, valid entry if not deleted and attribute not VOL and not LFN
            if (et != DDEM && attr != AM_LFN && (attr & AM_VOL) == 0) {
                if (compareFilenames(win, dp.dir_ptr, dp.fn) == 0) break;
            }
            res = dir_next(dp, false);
        } while (res == FResult.FR_OK);
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* Pick a top segment and create the object name in directory form       */
    /*-----------------------------------------------------------------------*/
    private FResult create_name(Dir dp, String[] pathRef) {
        byte c, d;
        byte[] sfn = dp.fn;
        int ni, si, i;
        String p = pathRef[0];
        // Non-LFN branch faithful to ff.c
        for (int k = 0; k < 11; k++) sfn[k] = (byte) ' ';
        sfn[NSFLAG] = 0;
        si = 0; i = 0; ni = 8;
        // Check dot entry (FF_FS_RPATH enabled? In our baked config FF_FS_RPATH=0, but keep handling for dot)
        if (p.length() > 0 && p.charAt(si) == '.') {
            // Is this a dot entry?
            // Copy one or two dots
            for (;;) {
                c = (byte) p.charAt(si);
                si++;
                if (c != '.' || si >= 3) break;
                sfn[i++] = c;
                if (si >= p.length()) break;
            }
            if (si < p.length()) {
                c = (byte) p.charAt(si);
                if (IsSeparator(c & 0xFF)) {
                    while (si < p.length() && IsSeparator(p.charAt(si) & 0xFF)) si++;
                    if (si >= p.length() || (byte) p.charAt(si) <= ' ') c = 0;
                    else c = (byte) p.charAt(si);
                } else if ((c & 0xFF) > ' ') {
                    return FResult.FR_INVALID_NAME;
                }
                // else c already holds terminator
            } else {
                c = 0;
            }
            pathRef[0] = p.substring(si);
            sfn[NSFLAG] = (byte) ((c <= ' ' ? NS_LAST : 0) | NS_DOT);
            return FResult.FR_OK;
        }
        // Create file name in directory form - non-LFN
        // Track NT flags
        int ntBody = 0; // bit 0: lower exists in body
        int ntExt = 0;  // bit 0: lower exists in ext
        boolean inExt = false;
        while (true) {
            if (si >= p.length()) {
                c = 0;
            } else {
                c = (byte) p.charAt(si++);
            }
            if ((c & 0xFF) <= ' ') break;             /* Break if end of the path name */
            if (IsSeparator(c & 0xFF)) {            /* Break if a separator is found */
                while (si < p.length() && IsSeparator(p.charAt(si) & 0xFF)) si++;    /* Skip duplicated separators */
                break;
            }
            if (c == '.' || i >= ni) {        /* End of body or field overflow? */
                if (ni == 11 || c != '.') return FResult.FR_INVALID_NAME;    /* Field overflow or invalid dot? */
                i = 8; ni = 11;                /* Enter file extension field */
                inExt = true;
                continue;
            }
            // SBCS extended char handling - we have no ExCvt for 932 DBCS only, so just keep
            if (dbc_1st(c & 0xFF)) {                /* Check if it is a DBC 1st byte */
                if (si >= p.length()) return FResult.FR_INVALID_NAME;
                d = (byte) p.charAt(si++);            /* Get 2nd byte */
                if (!dbc_2nd(d & 0xFF) || i >= ni - 1) return FResult.FR_INVALID_NAME;    /* Reject invalid DBC */
                sfn[i++] = c;
                sfn[i++] = d;
            } else {                        /* SBC */
                if ("*+,:;<=>[]|\"?\u007F".indexOf((char)(c & 0xFF)) >= 0) return FResult.FR_INVALID_NAME;    /* Reject illegal chrs for SFN */
                if (IsLower(c & 0xFF)) {
                    if (inExt) ntExt = NS_EXT; else ntBody = NS_BODY;
                    c = (byte) ((c & 0xFF) - 0x20);    /* To upper */
                }
                sfn[i++] = c;
            }
        }
        pathRef[0] = (si < p.length()) ? p.substring(si) : "";
        if (i == 0) return FResult.FR_INVALID_NAME;    /* Reject nul string */
        if ((sfn[0] & 0xFF) == DDEM) sfn[0] = (byte) RDDEM;    /* If the first character collides with DDEM, replace it with RDDEM */
        int cf = (c <= ' ' || (si < p.length() && p.charAt(si) <= ' ')) ? NS_LAST : 0;
        cf |= ntBody | ntExt;
        sfn[NSFLAG] = (byte) cf;    /* Set last segment flag if end of the path */
        return FResult.FR_OK;
    }

    /* Directory handling - Follow a path */
    private FResult follow_path(Dir dp, String path) {
        // path is already stripped of drive prefix, may contain leading separators
        // Determine start directory
        // With no RPATH, start at root always
        while (path.length() > 0 && IsSeparator(path.charAt(0))) path = path.substring(1);
        dp.sclust = 0; // root
        dp.clust = 0;
        dp.sect = 0;
        if (path.length() == 0 || path.charAt(0) == 0) {
            dp.fn[NSFLAG] = (byte) NS_NONAME;
            return dir_sdi(dp, 0);
        }
        String[] ref = new String[]{ path };
        for (;;) {
            FResult res = create_name(dp, ref);
            if (res != FResult.FR_OK) return res;
            int ns = dp.fn[NSFLAG] & 0xFF;
            // Dot handling: in root only "." and ".." are valid but ".." at root stays
            if ((ns & NS_DOT) != 0) {
                if ((ns & NS_LAST) != 0) {
                    dp.fn[NSFLAG] = (byte) NS_NONAME;
                    return dir_sdi(dp, 0);
                } else {
                    // Continue to next segment (stay at same directory, for root '.' means stay)
                    // For simplicity, if segment is "." just continue
                    if (dp.fn[0] == '.' && dp.fn[1] == ' ') {
                        if (ref[0].length() == 0) { dp.fn[NSFLAG] = (byte) NS_NONAME; return dir_sdi(dp, 0); }
                        continue;
                    }
                    // ".." at root stays at root
                    continue;
                }
            }
            res = dir_find(dp);
            if (res != FResult.FR_OK) {
                if (res == FResult.FR_NO_FILE) {
                    if ((ns & NS_LAST) == 0) res = FResult.FR_NO_PATH;
                }
                return res;
            }
            if ((ns & NS_LAST) != 0) break;
            // Not last: must be directory
            int attr = 0;
            // need to get attr from window
            FResult r = move_window(dp.sect);
            if (r != FResult.FR_OK) return r;
            attr = win[dp.dir_ptr + DIR_Attr] & 0xFF;
            if ((attr & AM_DIR) == 0) return FResult.FR_NO_PATH;
            dp.sclust = ldClustFromWin(dp.dir_ptr);
            // Continue loop to find next segment in sub-directory
            String remaining = ref[0];
            // keep dp.sclust for next iteration; dir_find will start from that directory
        }
        // At this point dp points to the object entry; preserve its position
        // Need to capture object info for Dir? For files, dp holds entry location
        // Move window to ensure win contains entry
        FResult res = move_window(dp.sect);
        if (res != FResult.FR_OK) return res;
        return FResult.FR_OK;
    }

    /* Check what the sector is */
    private int check_fs(long sect) {
        // returns 0: FAT VBR, 2: not FAT valid BS, 3: invalid, 4: disk error
        wflag = 0; winsect = -1;
        if (move_window(sect) != FResult.FR_OK) return 4;
        int sign = ld_16(win, BS_55AA);
        int b = win[BS_JmpBoot] & 0xFF;
        if (b == 0xEB || b == 0xE9 || b == 0xE8) {
            if (sign == 0xAA55 && win[BS_FilSysType32] == 'F') {
                // quick check for "FAT32   " or "FAT     "
                // Use generic check as in C: memcmp for FAT32 string
                boolean isFat32 = true;
                String fat32 = "FAT32   ";
                for (int i = 0; i < 8; i++) if ((win[BS_FilSysType32 + i] & 0xFF) != fat32.charAt(i)) { isFat32 = false; break; }
                if (isFat32) return 0;
            }
            int w = ld_16(win, BPB_BytsPerSec);
            int c = win[BPB_SecPerClus] & 0xFF;
            int rsv = ld_16(win, BPB_RsvdSecCnt);
            int nf = win[BPB_NumFATs] & 0xFF;
            int nroot = ld_16(win, BPB_RootEntCnt);
            int tot16 = ld_16(win, BPB_TotSec16);
            long tot32 = ld_32(win, BPB_TotSec32);
            int fatsz16 = ld_16(win, BPB_FATSz16);
            boolean sectOk = (w & (w - 1)) == 0 && w >= 512 && w <= 512;
            boolean clOk = c != 0 && (c & (c - 1)) == 0;
            boolean rsvOk = rsv != 0;
            boolean nfOk = nf == 1 || nf == 2;
            boolean rootOk = true; // for checkFS initial, root cnt not strictly checked? but we check alignment later
            boolean totOk = tot16 >= 128 || tot32 >= 0x10000;
            boolean fatszOk = fatsz16 != 0;
            if (sectOk && clOk && rsvOk && nfOk && totOk && fatszOk) return 0;
        }
        return sign == 0xAA55 ? 2 : 3;
    }

    /* Find an FAT volume */
    /* (It supports only generic partitioning rules, MBR, GPT and SFD) */
    private int find_volume() {
        int fmt = check_fs(0);
        if (fmt != 2) {
            if (fmt >= 3 || fmt == 0) return fmt; // as per C logic: if fmt !=2 && (fmt>=3 || part==0) return fmt; part==0 always
            // For fmt==2 we continue
        }
        // Need to examine MBR partition table
        // win currently holds sector 0 (MBR or VBR). If fmt==2 it's a valid BS not FAT, likely MBR.
        // Read partition entries
        long[] mbrPt = new long[4];
        for (int i = 0; i < 4; i++) {
            mbrPt[i] = ld_32(win, MBR_Table + i * SZ_PTE + PTE_StLba);
        }
        for (int i = 0; i < 4; i++) {
            if (mbrPt[i] == 0) continue;
            fmt = check_fs(mbrPt[i]);
            if (fmt == 0) return fmt; // found FAT
        }
        // No FAT partition found
        return 3;
    }

    /*-----------------------------------------------------------------------*/
    /* Determine logical drive number and mount the volume if needed         */
    /*-----------------------------------------------------------------------*/
    private FResult mount_volume(int mode) {
        // mode bits: write protection check if mode & ~FA_READ !=0
        boolean write = (mode & ~FA_READ) != 0;
        if (fs_type != 0) {
            int stat = diskIo.diskStatus();
            if ((stat & DiskIo.STA_NOINIT) == 0) {
                if (write && (stat & DiskIo.STA_PROTECT) != 0) return FResult.FR_WRITE_PROTECTED;
                return FResult.FR_OK;
            }
        }
        fs_type = 0;
        int stat = diskIo.diskInitialize();
        if ((stat & DiskIo.STA_NOINIT) != 0) return FResult.FR_NOT_READY;
        if (write && (stat & DiskIo.STA_PROTECT) != 0) return FResult.FR_WRITE_PROTECTED;
        // Find FAT volume
        int fmt = find_volume();
        if (fmt == 4) return FResult.FR_DISK_ERR;
        if (fmt >= 2) return FResult.FR_NO_FILESYSTEM;
        long bsect = winsect;
        // Initialize FS object based on BPB
        int bytsPerSec = ld_16(win, BPB_BytsPerSec);
        if (bytsPerSec != FF_MAX_SS) return FResult.FR_NO_FILESYSTEM;
        long fasize = ld_16(win, BPB_FATSz16) & 0xFFFFL;
        if (fasize == 0) fasize = ld_32(win, BPB_FATSz32);
        fsize = fasize;
        n_fats = win[BPB_NumFATs] & 0xFF;
        if (n_fats != 1 && n_fats != 2) return FResult.FR_NO_FILESYSTEM;
        long fasizeTotal = fasize * n_fats;
        csize = win[BPB_SecPerClus] & 0xFF;
        if (csize == 0 || (csize & (csize - 1)) != 0) return FResult.FR_NO_FILESYSTEM;
        n_rootdir = ld_16(win, BPB_RootEntCnt);
        if (n_rootdir % (FF_MAX_SS / SZDIRE) != 0) return FResult.FR_NO_FILESYSTEM;
        long tsect = ld_16(win, BPB_TotSec16) & 0xFFFFL;
        if (tsect == 0) tsect = ld_32(win, BPB_TotSec32);
        int nrsv = ld_16(win, BPB_RsvdSecCnt);
        if (nrsv == 0) return FResult.FR_NO_FILESYSTEM;
        long sysect = nrsv + fasizeTotal + n_rootdir / (FF_MAX_SS / SZDIRE);
        if (tsect < sysect) return FResult.FR_NO_FILESYSTEM;
        long nclst = (tsect - sysect) / csize;
        if (nclst == 0) return FResult.FR_NO_FILESYSTEM;
        int fmtType;
        if (nclst <= MAX_FAT12) fmtType = FS_FAT12;
        else if (nclst <= MAX_FAT16) fmtType = FS_FAT16;
        else if (nclst <= MAX_FAT32) fmtType = FS_FAT32;
        else return FResult.FR_NO_FILESYSTEM;
        // Boundaries
        n_fatent = nclst + 2;
        volbase = bsect;
        fatbase = bsect + nrsv;
        database = bsect + sysect;
        if (fmtType == FS_FAT32) {
            if (ld_16(win, BPB_FSVer32) != 0) return FResult.FR_NO_FILESYSTEM;
            if (n_rootdir != 0) return FResult.FR_NO_FILESYSTEM;
            dirbase = ld_32(win, BPB_RootClus32);
            long szbfat = n_fatent * 4;
            if (fsize < (szbfat + FF_MAX_SS - 1) / FF_MAX_SS) return FResult.FR_NO_FILESYSTEM;
        } else {
            if (n_rootdir == 0) return FResult.FR_NO_FILESYSTEM;
            dirbase = fatbase + fasizeTotal - (n_rootdir / (FF_MAX_SS / SZDIRE)) * 0 + fatbase + fasize * n_fats; // Actually dirbase = fatbase + fasize*n_fats
            dirbase = fatbase + fasize * n_fats;
            long szbfat;
            if (fmtType == FS_FAT16) szbfat = n_fatent * 2;
            else szbfat = n_fatent * 3 / 2 + (n_fatent & 1);
            if (fsize < (szbfat + FF_MAX_SS - 1) / FF_MAX_SS) return FResult.FR_NO_FILESYSTEM;
        }
        // FSInfo for FAT32
        last_clst = 0xFFFFFFFFL;
        free_clst = 0xFFFFFFFFL;
        fsi_flag = 0x80;
        if (fmtType == FS_FAT32 && ld_16(win, BPB_FSInfo32) == 1) {
            if (move_window(bsect + 1) == FResult.FR_OK) {
                if (ld_32(win, FSI_LeadSig) == 0x41615252L && ld_32(win, FSI_StrucSig) == 0x61417272L && ld_32(win, FSI_TrailSig) == 0xAA550000L) {
                    free_clst = ld_32(win, FSI_Free_Count);
                    last_clst = ld_32(win, FSI_Nxt_Free);
                }
                fsi_flag = 0;
            }
        }
        fs_type = fmtType;
        id = ++Fsid;
        wflag = 0;
        fsi_flag &= 0x7F; // enable? In C after mount, fsi_flag =0 if FAT32 else 0x80? We'll set to 0 for now
        if (fmtType != FS_FAT32) fsi_flag = 0x80;
        else fsi_flag = 0;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Check if the file/directory object is valid or not                    */
    /*-----------------------------------------------------------------------*/
    private FResult validate(int objId, FatFs objFs) {
        if (objFs == null || objFs != this || objFs.fs_type == 0 || objId != id) return FResult.FR_INVALID_OBJECT;
        int stat = diskIo.diskStatus();
        if ((stat & DiskIo.STA_NOINIT) != 0) return FResult.FR_NOT_READY;
        return FResult.FR_OK;
    }

    private FResult validate(Fil fp) {
        if (fp == null || fp.fs == null) return FResult.FR_INVALID_OBJECT;
        return validate(fp.id, fp.fs);
    }
    private FResult validate(Dir dp) {
        if (dp == null || dp.fs == null) return FResult.FR_INVALID_OBJECT;
        return validate(dp.id, dp.fs);
    }

    /*-----------------------------------------------------------------------*/
    /* Create partitions on the physical drive in format of MBR or GPT       */
    /*-----------------------------------------------------------------------*/
    private FResult createPartition(long[] plst, int sys, byte[] workBuf) {
        /* Get physical drive size */
        long[] szDrvRef = new long[1];
        DResult dr = diskIo.diskIoctl(DiskIo.GET_SECTOR_COUNT, szDrvRef);
        long szDrv;
        if (dr == DResult.RES_OK) {
            szDrv = szDrvRef[0];
        } else {
            int[] tmp = new int[1];
            if (diskIo.diskIoctl(DiskIo.GET_SECTOR_COUNT, tmp) != DResult.RES_OK) return FResult.FR_DISK_ERR;
            szDrv = tmp[0] & 0xFFFFFFFFL;
        }
        /* Create partitions in MBR format */
        long szDrv32 = szDrv & 0xFFFFFFFFL;
        int nSc = N_SEC_TRACK; /* Determine drive CHS without any consideration of the drive geometry */
        int nHd = 8;
        while (nHd != 0 && szDrv32 / nHd / nSc > 1024) {
            nHd <<= 1;
            if (nHd >= 256) nHd = 0;
        }
        if (nHd == 0) nHd = 255; /* Number of heads needs to be <256 */

        Arrays.fill(workBuf, (byte) 0); /* Clear MBR */
        int pte = MBR_Table; /* Partition table in the MBR */
        long nxtAlloc32 = nSc;
        for (int i = 0; i < 4 && nxtAlloc32 != 0 && nxtAlloc32 < szDrv32; i++) {
            long szPart32 = (i < plst.length) ? (plst[i] & 0xFFFFFFFFL) : 0; /* Get partition size */
            if (szPart32 <= 100) szPart32 = (szPart32 == 100) ? szDrv32 : szDrv32 / 100 * szPart32; /* Size in percentage? */
            if (nxtAlloc32 + szPart32 > szDrv32 || nxtAlloc32 + szPart32 < nxtAlloc32) szPart32 = szDrv32 - nxtAlloc32; /* Clip at drive size */
            if (szPart32 == 0) break; /* End of table or no sector to allocate? */

            st_32(workBuf, pte + PTE_StLba, nxtAlloc32); /* Partition start LBA sector */
            st_32(workBuf, pte + PTE_SizLba, szPart32); /* Size of partition [sector] */
            workBuf[pte + PTE_System] = (byte) sys; /* System type */

            int cy = (int) (nxtAlloc32 / nSc / nHd); /* Partition start CHS cylinder */
            int hd = (int) (nxtAlloc32 / nSc % nHd); /* Partition start CHS head */
            int sc = (int) (nxtAlloc32 % nSc + 1); /* Partition start CHS sector */
            workBuf[pte + PTE_StHead] = (byte) hd;
            workBuf[pte + PTE_StSec] = (byte) ((cy >> 2 & 0xC0) | sc);
            workBuf[pte + PTE_StCyl] = (byte) cy;

            cy = (int) ((nxtAlloc32 + szPart32 - 1) / nSc / nHd); /* Partition end CHS cylinder */
            hd = (int) ((nxtAlloc32 + szPart32 - 1) / nSc % nHd); /* Partition end CHS head */
            sc = (int) ((nxtAlloc32 + szPart32 - 1) % nSc + 1); /* Partition end CHS sector */
            workBuf[pte + PTE_EdHead] = (byte) hd;
            workBuf[pte + PTE_EdSec] = (byte) ((cy >> 2 & 0xC0) | sc);
            workBuf[pte + PTE_EdCyl] = (byte) cy;

            pte += SZ_PTE; /* Next entry */
            nxtAlloc32 += szPart32;
        }

        st_16(workBuf, BS_55AA, 0xAA55); /* MBR signature */
        if (diskIo.diskWrite(0, workBuf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR; /* Write it to the MBR */

        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Create FAT/exFAT volume                                          */
    /*-----------------------------------------------------------------------*/
    /**
     * Create FAT volume. Deviation from C signature {@code f_mkfs(path, opt, work, len)}:
     * work/len parameters are omitted; scratch buffer (one 512-byte sector) is allocated internally.
     */
    public FResult mkfs(String path, MkfsParm opt) {
        /* Check mounted drive and clear work area */
        String[] _remArr = new String[]{path};

        int _remVol = get_ldnumber(_remArr);

        if (_remVol < 0) return FResult.FR_INVALID_DRIVE;

        String rem = _remArr[0];
        this.fs_type = 0; /* Clear the fs object if mounted */
        this.winsect = -1;
        this.wflag = 0;
        this.fsi_flag = 0;

        /* Initialize the hosting physical drive */
        int ds = diskIo.diskInitialize();
        if ((ds & DiskIo.STA_NOINIT) != 0) return FResult.FR_NOT_READY;
        if ((ds & DiskIo.STA_PROTECT) != 0) return FResult.FR_WRITE_PROTECTED;

        /* Get physical drive parameters (sz_blk and ss) */
        if (opt == null) opt = MkfsParm.defaultParm(); /* Use default parameter if it is not given */
        long szBlk = opt.align & 0xFFFFFFFFL;
        if (szBlk == 0) {
            long[] blkRef = new long[1];
            DResult blkRes = diskIo.diskIoctl(DiskIo.GET_BLOCK_SIZE, blkRef);
            if (blkRes == DResult.RES_OK) {
                szBlk = blkRef[0];
            } else {
                /* Try int[] fallback for backward compatibility */
                int[] tmp = new int[1];
                if (diskIo.diskIoctl(DiskIo.GET_BLOCK_SIZE, tmp) == DResult.RES_OK) {
                    szBlk = tmp[0] & 0xFFFFFFFFL;
                } else {
                    szBlk = 0;
                }
            }
        } /* Block size from the parameter or lower layer */
        if (szBlk == 0 || szBlk > 0x8000 || (szBlk & (szBlk - 1)) != 0) szBlk = 1; /* Use default if the block size is invalid */
        int ss = FF_MAX_SS;

        /* Options for FAT sub-type and FAT parameters */
        int fsopt = opt.fmt & (FM_ANY | FM_SFD);
        int nFat = (opt.nFat >= 1 && opt.nFat <= 2) ? opt.nFat : 1;
        int nRoot = (opt.nRoot >= 1 && opt.nRoot <= 32768 && (opt.nRoot % (ss / SZDIRE)) == 0) ? opt.nRoot : 512;
        long szAu = (opt.auSize <= 0x1000000 && (opt.auSize & (opt.auSize - 1)) == 0) ? opt.auSize : 0;
        szAu /= ss; /* Byte --> Sector */

        /* Get working buffer */
        byte[] buf = new byte[FF_MAX_SS]; /* Working buffer */
        long szBuf = 1; /* Size of working buffer [sector] - deviation: fixed to 1 sector (C uses len/ss) */

        /* Determine where the volume to be located (b_vol, sz_vol) */
        long bVol = 0;
        long szVol = 0;
        /* The volume is associated with a physical drive */
        {
            long[] volRef = new long[1];
            DResult volRes = diskIo.diskIoctl(DiskIo.GET_SECTOR_COUNT, volRef);
            if (volRes != DResult.RES_OK) {
                int[] tmp = new int[1];
                if (diskIo.diskIoctl(DiskIo.GET_SECTOR_COUNT, tmp) == DResult.RES_OK) {
                    szVol = tmp[0] & 0xFFFFFFFFL;
                } else {
                    return FResult.FR_DISK_ERR;
                }
            } else {
                szVol = volRef[0];
            }
            if ((fsopt & FM_SFD) == 0) { /* To be partitioned? */
                /* Create a single-partition on the drive in this function */
                /* Partitioning is in MBR */
                if (szVol > N_SEC_TRACK) {
                    bVol = N_SEC_TRACK; szVol -= bVol; /* Estimated partition offset and size */
                }
            }
        }
        if (szVol < 128) return FResult.FR_MKFS_ABORTED; /* Check if volume size is >=128 sectors */

        /* Now start to create an FAT volume at b_vol and sz_vol */

        int fsty;
        do { /* Pre-determine the FAT type */
            if (szAu > 128) szAu = 128; /* Invalid AU for FAT/FAT32? */
            if ((fsopt & FM_FAT32) != 0) { /* FAT32 possible? */
                if ((fsopt & FM_FAT) == 0) { /* no-FAT? */
                    fsty = FS_FAT32; break;
                }
            }
            if ((fsopt & FM_FAT) == 0) return FResult.FR_INVALID_PARAMETER; /* no-FAT? */
            fsty = FS_FAT16;
        } while (false);

        long vsn = (szVol + get_fattime()) & 0xFFFFFFFFL; /* VSN generated from current time and partition size */

        /* Create an FAT/FAT32 volume */
        long pau;
        long nClst;
        long szFat;
        long szRsv;
        long szDir;
        long bFat;
        long bData;
        long sect;
        long nsect;
        long n;
        int i;
        {
            do {
                pau = szAu;
                /* Pre-determine number of clusters and FAT sub-type */
                if (fsty == FS_FAT32) { /* FAT32 volume */
                    if (pau == 0) { /* AU auto-selection */
                        n = szVol / 0x20000; /* Volume size in unit of 128KS */
                        for (i = 0, pau = 1; cst32[i] != 0 && cst32[i] <= n; i++, pau <<= 1) ; /* Get from table */
                    }
                    nClst = szVol / pau; /* Number of clusters */
                    szFat = (nClst * 4 + 8 + ss - 1) / ss; /* FAT size [sector] */
                    szRsv = 32; /* Number of reserved sectors */
                    szDir = 0; /* No static directory */
                    if (nClst <= MAX_FAT16 || nClst > MAX_FAT32) return FResult.FR_MKFS_ABORTED;
                } else { /* FAT volume */
                    if (pau == 0) { /* au auto-selection */
                        n = szVol / 0x1000; /* Volume size in unit of 4KS */
                        for (i = 0, pau = 1; cst[i] != 0 && cst[i] <= n; i++, pau <<= 1) ; /* Get from table */
                    }
                    nClst = szVol / pau;
                    if (nClst > MAX_FAT12) {
                        n = nClst * 2 + 4; /* FAT size [byte] */
                    } else {
                        fsty = FS_FAT12;
                        n = (nClst * 3 + 1) / 2 + 3; /* FAT size [byte] */
                    }
                    szFat = (n + ss - 1) / ss; /* FAT size [sector] */
                    szRsv = 1; /* Number of reserved sectors */
                    szDir = (long) nRoot * SZDIRE / ss; /* Root directory size [sector] */
                }
                bFat = bVol + szRsv; /* FAT base */
                bData = bFat + szFat * nFat + szDir; /* Data base */

                /* Align data area to erase block boundary (for flash memory media) */
                n = (((bData + szBlk - 1) & ~(szBlk - 1)) - bData); /* Sectors to next nearest from current data base */
                if (fsty == FS_FAT32) { /* FAT32: Move FAT */
                    szRsv += n; bFat += n;
                } else { /* FAT: Expand FAT */
                    if (n % nFat != 0) { /* Adjust fractional error if needed */
                        n--; szRsv++; bFat++;
                    }
                    szFat += n / nFat;
                }

                /* Determine number of clusters and final check of validity of the FAT sub-type */
                if (szVol < bData + pau * 16 - bVol) return FResult.FR_MKFS_ABORTED; /* Too small volume? */
                nClst = (szVol - szRsv - szFat * nFat - szDir) / pau;
                if (fsty == FS_FAT32) {
                    if (nClst <= MAX_FAT16) { /* Too few clusters for FAT32? */
                        if (szAu == 0 && (szAu = pau / 2) != 0) continue; /* Adjust cluster size and retry */
                        return FResult.FR_MKFS_ABORTED;
                    }
                }
                if (fsty == FS_FAT16) {
                    if (nClst > MAX_FAT16) { /* Too many clusters for FAT16 */
                        if (szAu == 0 && (pau * 2) <= 64) {
                            szAu = pau * 2; continue; /* Adjust cluster size and retry */
                        }
                        if ((fsopt & FM_FAT32) != 0) {
                            fsty = FS_FAT32; continue; /* Switch type to FAT32 and retry */
                        }
                        if (szAu == 0 && (szAu = pau * 2) <= 128) continue; /* Adjust cluster size and retry */
                        return FResult.FR_MKFS_ABORTED;
                    }
                    if (nClst <= MAX_FAT12) { /* Too few clusters for FAT16 */
                        if (szAu == 0 && (szAu = pau * 2) <= 128) continue; /* Adjust cluster size and retry */
                        return FResult.FR_MKFS_ABORTED;
                    }
                }
                if (fsty == FS_FAT12 && nClst > MAX_FAT12) return FResult.FR_MKFS_ABORTED; /* Too many clusters for FAT12 */

                /* Ok, it is the valid cluster configuration */
                break;
            } while (true);
        }

        /* Create FAT VBR */
        Arrays.fill(buf, (byte) 0);
        /* Boot jump code (x86), OEM name */
        byte[] jumpOem = new byte[]{(byte) 0xEB, (byte) 0xFE, (byte) 0x90, (byte) 'M', (byte) 'S', (byte) 'D', (byte) 'O', (byte) 'S', (byte) '5', (byte) '.', (byte) '0'};
        System.arraycopy(jumpOem, 0, buf, BS_JmpBoot, 11); /* Boot jump code (x86), OEM name */
        st_16(buf, BPB_BytsPerSec, ss); /* Sector size [byte] */
        buf[BPB_SecPerClus] = (byte) pau; /* Cluster size [sector] */
        st_16(buf, BPB_RsvdSecCnt, (int) szRsv); /* Size of reserved area */
        buf[BPB_NumFATs] = (byte) nFat; /* Number of FATs */
        st_16(buf, BPB_RootEntCnt, (fsty == FS_FAT32) ? 0 : nRoot); /* Number of root directory entries */
        if (szVol < 0x10000) {
            st_16(buf, BPB_TotSec16, (int) szVol); /* Volume size in 16-bit LBA */
        } else {
            st_32(buf, BPB_TotSec32, szVol); /* Volume size in 32-bit LBA */
        }
        buf[BPB_Media] = (byte) 0xF8; /* Media descriptor byte */
        st_16(buf, BPB_SecPerTrk, 63); /* Number of sectors per track (for int13) */
        st_16(buf, BPB_NumHeads, 255); /* Number of heads (for int13) */
        st_32(buf, BPB_HiddSec, bVol); /* Volume offset in the physical drive [sector] */
        if (fsty == FS_FAT32) {
            st_32(buf, BS_VolID32, vsn); /* VSN */
            st_32(buf, BPB_FATSz32, szFat); /* FAT size [sector] */
            st_32(buf, BPB_RootClus32, 2); /* Root directory cluster # (2) */
            st_16(buf, BPB_FSInfo32, 1); /* Offset of FSINFO sector (VBR + 1) */
            st_16(buf, BPB_BkBootSec32, 6); /* Offset of backup VBR (VBR + 6) */
            buf[BS_DrvNum32] = (byte) 0x80; /* Drive number (for int13) */
            buf[BS_BootSig32] = (byte) 0x29; /* Extended boot signature */
            byte[] volLabel = "NO NAME    FAT32   ".getBytes(java.nio.charset.StandardCharsets.US_ASCII);
            System.arraycopy(volLabel, 0, buf, BS_VolLab32, 19); /* Volume label, FAT signature */
        } else {
            st_32(buf, BS_VolID, vsn); /* VSN */
            st_16(buf, BPB_FATSz16, (int) szFat); /* FAT size [sector] */
            buf[BS_DrvNum] = (byte) 0x80; /* Drive number (for int13) */
            buf[BS_BootSig] = (byte) 0x29; /* Extended boot signature */
            byte[] volLabel = "NO NAME    FAT     ".getBytes(java.nio.charset.StandardCharsets.US_ASCII);
            System.arraycopy(volLabel, 0, buf, BS_VolLab, 19); /* Volume label, FAT signature */
        }
        st_16(buf, BS_55AA, 0xAA55); /* Signature (offset is fixed here regardless of sector size) */
        if (diskIo.diskWrite(bVol, buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR; /* Write it to the VBR sector */

        /* Create FSINFO record if needed */
        if (fsty == FS_FAT32) {
            if (diskIo.diskWrite(bVol + 6, buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR; /* Write backup VBR (VBR + 6) */
            Arrays.fill(buf, (byte) 0);
            st_32(buf, FSI_LeadSig, 0x41615252L);
            st_32(buf, FSI_StrucSig, 0x61417272L);
            st_32(buf, FSI_Free_Count, nClst - 1); /* Number of free clusters */
            st_32(buf, FSI_Nxt_Free, 2); /* Last allocated cluster# */
            st_16(buf, BS_55AA, 0xAA55);
            if (diskIo.diskWrite(bVol + 7, buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR; /* Write backup FSINFO (VBR + 7) */
            if (diskIo.diskWrite(bVol + 1, buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR; /* Write original FSINFO (VBR + 1) */
        }

        /* Initialize FAT area */
        Arrays.fill(buf, (byte) 0);
        sect = bFat; /* FAT start sector */
        for (i = 0; i < nFat; i++) { /* Initialize FATs each */
            if (fsty == FS_FAT32) {
                st_32(buf, 0, 0xFFFFFFF8L); /* FAT[0] */
                st_32(buf, 4, 0xFFFFFFFFL); /* FAT[1] */
                st_32(buf, 8, 0x0FFFFFFFL); /* FAT[2] (root directory at cluster# 2) */
            } else {
                st_32(buf, 0, (fsty == FS_FAT12) ? 0xFFFFF8L : 0xFFFFFFF8L); /* FAT[0] and FAT[1] */
            }
            nsect = szFat; /* Number of FAT sectors */
            do { /* Fill FAT sectors */
                n = (nsect > szBuf) ? szBuf : nsect;
                if (diskIo.diskWrite(sect, buf, (int) n) != DResult.RES_OK) return FResult.FR_DISK_ERR;
                Arrays.fill(buf, (byte) 0); /* Rest of FAT area is initially zero */
                sect += n; nsect -= n;
            } while (nsect != 0);
            /* For next FAT, need to re-init first sector header if there is another FAT */
            if (i + 1 < nFat) {
                /* buf is already zeroed, next loop will set header again */
            }
        }

        /* Initialize root directory (fill with zero) */
        nsect = (fsty == FS_FAT32) ? pau : szDir; /* Number of root directory sectors */
        do {
            n = (nsect > szBuf) ? szBuf : nsect;
            if (diskIo.diskWrite(sect, buf, (int) n) != DResult.RES_OK) return FResult.FR_DISK_ERR;
            sect += n; nsect -= n;
        } while (nsect != 0);

        /* A FAT volume has been created here */

        /* Determine system ID in the MBR partition table */
        int sys;
        if (fsty == FS_FAT32) {
            sys = 0x0C; /* FAT32X */
        } else if (szVol >= 0x10000) {
            sys = 0x06; /* FAT12/16 (large) */
        } else if (fsty == FS_FAT16) {
            sys = 0x04; /* FAT16 */
        } else {
            sys = 0x01; /* FAT12 */
        }

        /* Update partition information */
        /* Volume as a new single partition */
        if ((fsopt & FM_SFD) == 0) { /* Create partition table if not in SFD format */
            long[] lba = new long[]{szVol, 0};
            FResult res = createPartition(lba, sys, buf);
            if (res != FResult.FR_OK) return res;
        }

        if (diskIo.diskIoctl(DiskIo.CTRL_SYNC, null) != DResult.RES_OK) return FResult.FR_DISK_ERR;

        return FResult.FR_OK;
    }

    //------------------- public API -------------------
    /*-----------------------------------------------------------------------*/
    /* API: Mount/Unmount a Logical Drive                                    */
    /*-----------------------------------------------------------------------*/
    /**
     * f_mount wrapper – mount volume (drive 0) immediately. (Mount/Unmount a logical drive)
     */
    public FResult mount() {
        return mount(null, 1);
    }

    /**
     * f_mount wrapper with drive prefix handling.
     * @param path drive prefix like "0:" or null for default
     * @param opt  0 = do not mount (or unmount), 1 = mount immediately
     */
    public FResult mount(String path, int opt) {
        String[] _remainderArr = new String[]{path};

        int _remainderVol = get_ldnumber(_remainderArr);

        if (_remainderVol < 0) return FResult.FR_INVALID_DRIVE;

        String remainder = _remainderArr[0];
        if (opt == 0) {
            fs_type = 0;
            winsect = -1;
            wflag = 0;
            return FResult.FR_OK;
        }
        // opt ==1: mount
        FResult res = mount_volume(0);
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Open or Create a File                                            */
    /*-----------------------------------------------------------------------*/
    /**
     * f_open – open or create a file. (Open or create a file)
     */
    public FResult open(Fil fp, String path, int mode) {
        if (fp == null) return FResult.FR_INVALID_OBJECT;
        String[] _strippedArr = new String[]{path};

        int _strippedVol = get_ldnumber(_strippedArr);

        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String stripped = _strippedArr[0];
        // Mask mode
        mode &= (FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS | FA_OPEN_APPEND);
        // Mount volume if needed
        FResult res = mount_volume(mode);
        if (res != FResult.FR_OK) {
            fp.fs = null;
            return res;
        }
        Dir dj = new Dir();
        dj.fs = this;
        res = follow_path(dj, stripped);
        // For create modes, handle not found
        if ((mode & (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW)) != 0) {
            if (res != FResult.FR_OK) {
                if (res == FResult.FR_NO_FILE) {
                    // Create new entry
                    res = dir_register(dj);
                    mode |= FA_CREATE_ALWAYS;
                } else {
                    fp.fs = null;
                    return res;
                }
            } else {
                // Object exists
                if ((mode & FA_CREATE_NEW) != 0) {
                    fp.fs = null;
                    return FResult.FR_EXIST;
                }
                // Check RDO/DIR
                res = move_window(dj.sect);
                if (res != FResult.FR_OK) { fp.fs = null; return res; }
                int attr = win[dj.dir_ptr + DIR_Attr] & 0xFF;
                if ((attr & (AM_RDO | AM_DIR)) != 0) {
                    fp.fs = null;
                    return FResult.FR_DENIED;
                }
                if ((mode & FA_CREATE_ALWAYS) != 0) {
                    // Truncate
                    long tm = get_fattime();
                    // Clear dir entry and cluster chain
                    long cl = ldClustFromWin(dj.dir_ptr);
                    st_32(win, dj.dir_ptr + DIR_CrtTime, tm);
                    st_32(win, dj.dir_ptr + DIR_ModTime, tm);
                    win[dj.dir_ptr + DIR_Attr] = (byte) AM_ARC;
                    stClustWin(dj.dir_ptr, 0);
                    st_32(win, dj.dir_ptr + DIR_FileSize, 0);
                    wflag = 1;
                    long sc = winsect;
                    if (cl != 0) {
                        res = remove_chain(cl, 0);
                        if (res == FResult.FR_OK) {
                            res = move_window(sc);
                            if (res != FResult.FR_OK) { fp.fs = null; return res; }
                            last_clst = cl - 1;
                        } else {
                            fp.fs = null;
                            return res;
                        }
                    }
                }
            }
            if (res == FResult.FR_OK && (mode & FA_CREATE_ALWAYS) != 0) mode |= FA_MODIFIED;
            fp.dir_sect = winsect;
            fp.dir_ptr = dj.dir_ptr;
        } else {
            // Open existing
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            res = move_window(dj.sect);
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            int attr = win[dj.dir_ptr + DIR_Attr] & 0xFF;
            if ((attr & AM_DIR) != 0) { fp.fs = null; return FResult.FR_NO_FILE; }
            if ((mode & FA_WRITE) != 0 && (attr & AM_RDO) != 0) { fp.fs = null; return FResult.FR_DENIED; }
            fp.dir_sect = winsect;
            fp.dir_ptr = dj.dir_ptr;
        }
        if (res == FResult.FR_OK) {
            // Fill Fil object
            res = move_window(dj.sect);
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            fp.fs = this;
            fp.id = id;
            fp.attr = win[dj.dir_ptr + DIR_Attr] & 0xFF;
            fp.sclust = ldClustFromWin(dj.dir_ptr);
            fp.objsize = ld_32(win, dj.dir_ptr + DIR_FileSize);
            fp.flag = mode;
            fp.err = 0;
            fp.fptr = 0;
            fp.clust = 0;
            fp.sect = 0;
            for (int i = 0; i < fp.buf.length; i++) fp.buf[i] = 0;
            if ((mode & FA_SEEKEND) != 0 && fp.objsize > 0) {
                // Actually FA_OPEN_APPEND includes SEEKEND 0x20 + 0x10? But our FA_OPEN_APPEND =0x30 includes both bits. Need to seek to end.
                fp.fptr = fp.objsize;
                long bcs = (long) csize * FF_MAX_SS;
                long clst = fp.sclust;
                long ofs = fp.objsize;
                while (ofs > bcs) {
                    ofs -= bcs;
                    long nxt = get_fat(clst);
                    if (nxt <= 1) { fp.fs = null; return FResult.FR_INT_ERR; }
                    if (nxt == 0xFFFFFFFFL) { fp.fs = null; return FResult.FR_DISK_ERR; }
                    clst = nxt;
                }
                fp.clust = clst;
                if (ofs % FF_MAX_SS != 0) {
                    long sec = clst2sect(clst);
                    if (sec == 0) { fp.fs = null; return FResult.FR_INT_ERR; }
                    sec += (ofs / FF_MAX_SS);
                    // Fill cache if needed
                    if (diskIo.diskRead(sec, fp.buf, 1) != DResult.RES_OK) { fp.fs = null; return FResult.FR_DISK_ERR; }
                    fp.sect = sec;
                }
            }
        }
        if (res != FResult.FR_OK) fp.fs = null;
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Close File                                                       */
    /*-----------------------------------------------------------------------*/
    public FResult close(Fil fp) {
        if (fp == null) return FResult.FR_INVALID_OBJECT;
        FResult res = sync(fp);
        if (res != FResult.FR_OK) return res;
        res = validate(fp);
        if (res != FResult.FR_OK) return res;
        fp.fs = null;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Read File                                                        */
    /*-----------------------------------------------------------------------*/
    public FResult read(Fil fp, ByteBuffer buff, int btr, int[] br) {
        if (br != null) br[0] = 0;
        FResult res = validate(fp);
        if (res != FResult.FR_OK) return res;
        if (fp.err != 0) return FResult.values()[fp.err];
        if ((fp.flag & FA_READ) == 0) return FResult.FR_DENIED;
        long remain = fp.objsize - fp.fptr;
        if (btr > remain) btr = (int) remain;
        int rbuffOff = 0;
        int brCnt = 0;
        for (; btr > 0; ) {
            if (fp.fptr % FF_MAX_SS == 0) {
                int csect = (int) ((fp.fptr / FF_MAX_SS) & (csize - 1));
                if (csect == 0) {
                    long clst;
                    if (fp.fptr == 0) {
                        clst = fp.sclust;
                    } else {
                        clst = get_fat(fp.clust);
                    }
                    if (clst < 2) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                    if (clst == 0xFFFFFFFFL) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    fp.clust = clst;
                }
                long sect = clst2sect(fp.clust);
                if (sect == 0) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                sect += csect;
                int cc = btr / FF_MAX_SS;
                if (cc > 0) {
                    if (csect + cc > csize) cc = csize - csect;
                    // Direct read
                    byte[] tmp = new byte[cc * FF_MAX_SS];
                    if (diskIo.diskRead(sect, tmp, cc) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    // Handle dirty cache replacement? If our private cache overlaps, replace.
                    if ((fp.flag & FA_DIRTY) != 0 && fp.sect >= sect && fp.sect < sect + cc) {
                        int idx = (int)(fp.sect - sect);
                        System.arraycopy(fp.buf, 0, tmp, idx * FF_MAX_SS, FF_MAX_SS);
                    }
                    for (int i = 0; i < cc * FF_MAX_SS; i++) buff.put(rbuffOff + i, tmp[i]);
                    int rcnt = FF_MAX_SS * cc;
                    btr -= rcnt; rbuffOff += rcnt; brCnt += rcnt; fp.fptr += rcnt;
                    continue;
                }
                if (fp.sect != sect) {
                    if ((fp.flag & FA_DIRTY) != 0) {
                        if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                        fp.flag &= ~FA_DIRTY;
                    }
                    if (diskIo.diskRead(sect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    fp.sect = sect;
                }
            }
            int rcnt = FF_MAX_SS - (int)(fp.fptr % FF_MAX_SS);
            if (rcnt > btr) rcnt = btr;
            for (int i = 0; i < rcnt; i++) buff.put(rbuffOff + i, fp.buf[(int)(fp.fptr % FF_MAX_SS) + i]);
            btr -= rcnt; rbuffOff += rcnt; brCnt += rcnt; fp.fptr += rcnt;
        }
        if (br != null) br[0] = brCnt;
        return FResult.FR_OK;
    }

    public FResult read(Fil fp, byte[] buff, int btr, int[] br) {
        return read(fp, ByteBuffer.wrap(buff), btr, br);
    }

    /*-----------------------------------------------------------------------*/
    /* API: Write File                                                       */
    /*-----------------------------------------------------------------------*/
    public FResult write(Fil fp, ByteBuffer buff, int btw, int[] bw) {
        if (bw != null) bw[0] = 0;
        FResult res = validate(fp);
        if (res != FResult.FR_OK) return res;
        if (fp.err != 0) return FResult.values()[fp.err];
        if ((fp.flag & FA_WRITE) == 0) return FResult.FR_DENIED;
        if (fp.fptr + btw < fp.fptr) btw = (int)(0xFFFFFFFFL - fp.fptr);
        int wOff = 0;
        int bwCnt = 0;
        for (; btw > 0; ) {
            if (fp.fptr % FF_MAX_SS == 0) {
                int csect = (int)((fp.fptr / FF_MAX_SS) & (csize - 1));
                if (csect == 0) {
                    long clst;
                    if (fp.fptr == 0) {
                        clst = fp.sclust;
                        if (clst == 0) clst = create_chain(0);
                    } else {
                        clst = create_chain(fp.clust);
                    }
                    if (clst == 0) break;
                    if (clst == 1) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                    if (clst == 0xFFFFFFFFL) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    fp.clust = clst;
                    if (fp.sclust == 0) fp.sclust = clst;
                }
                if ((fp.flag & FA_DIRTY) != 0) {
                    if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    fp.flag &= ~FA_DIRTY;
                }
                long sect = clst2sect(fp.clust);
                if (sect == 0) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                sect += csect;
                int cc = btw / FF_MAX_SS;
                if (cc > 0) {
                    if (csect + cc > csize) cc = csize - csect;
                    byte[] slice = new byte[cc * FF_MAX_SS];
                    for (int i = 0; i < cc * FF_MAX_SS; i++) slice[i] = buff.get(wOff + i);
                    if (diskIo.diskWrite(sect, slice, cc) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    if (fp.sect >= sect && fp.sect < sect + cc) {
                        System.arraycopy(slice, (int)((fp.sect - sect) * FF_MAX_SS), fp.buf, 0, FF_MAX_SS);
                        fp.flag &= ~FA_DIRTY;
                    }
                    int wcnt = FF_MAX_SS * cc;
                    btw -= wcnt; wOff += wcnt; bwCnt += wcnt; fp.fptr += wcnt;
                    if (fp.fptr > fp.objsize) fp.objsize = fp.fptr;
                    continue;
                }
                if (fp.sect != sect && fp.fptr < fp.objsize) {
                    if (diskIo.diskRead(sect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                } else if (fp.sect != sect) {
                    // On growing edge, zero buffer
                    for (int i = 0; i < fp.buf.length; i++) fp.buf[i] = 0;
                }
                fp.sect = sect;
            }
            int wcnt = FF_MAX_SS - (int)(fp.fptr % FF_MAX_SS);
            if (wcnt > btw) wcnt = btw;
            for (int i = 0; i < wcnt; i++) fp.buf[(int)(fp.fptr % FF_MAX_SS) + i] = buff.get(wOff + i);
            fp.flag |= FA_DIRTY;
            btw -= wcnt; wOff += wcnt; bwCnt += wcnt; fp.fptr += wcnt;
            if (fp.fptr > fp.objsize) fp.objsize = fp.fptr;
        }
        fp.flag |= FA_MODIFIED;
        if (bw != null) bw[0] = bwCnt;
        return FResult.FR_OK;
    }

    public FResult write(Fil fp, byte[] buff, int btw, int[] bw) {
        return write(fp, ByteBuffer.wrap(buff), btw, bw);
    }

    // Special handling for write direct second overload to avoid slice misuse
    private FResult writeInternal(Fil fp, byte[] buff, int btw, int[] bw) { return write(fp, ByteBuffer.wrap(buff), btw, bw); }

    /*-----------------------------------------------------------------------*/
    /* API: Seek File Read/Write Pointer                                     */
    /*-----------------------------------------------------------------------*/
    public FResult lseek(Fil fp, long ofs) {
        FResult res = validate(fp);
        if (res != FResult.FR_OK) return res;
        if (fp.err != 0) return FResult.values()[fp.err];
        if (ofs > fp.objsize && (fp.flag & FA_WRITE) == 0) ofs = fp.objsize;
        long ifptr = fp.fptr;
        fp.fptr = 0;
        long nsect = 0;
        if (ofs > 0) {
            long bcs = (long) csize * FF_MAX_SS;
            long clst;
            if (ifptr > 0 && (ofs - 1) / bcs >= (ifptr - 1) / bcs) {
                fp.fptr = (ifptr - 1) & ~(bcs - 1);
                ofs -= fp.fptr;
                clst = fp.clust;
            } else {
                clst = fp.sclust;
                if (clst == 0) {
                    clst = create_chain(0);
                    if (clst == 1) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                    if (clst == 0xFFFFFFFFL) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    fp.sclust = clst;
                }
                fp.clust = clst;
            }
            if (clst != 0) {
                while (ofs > bcs) {
                    ofs -= bcs; fp.fptr += bcs;
                    if ((fp.flag & FA_WRITE) != 0) {
                        if (fp.fptr > fp.objsize) { fp.objsize = fp.fptr; fp.flag |= FA_MODIFIED; }
                        clst = create_chain(clst);
                        if (clst == 0) { ofs = 0; break; }
                    } else {
                        clst = get_fat(clst);
                    }
                    if (clst == 0xFFFFFFFFL) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                    if (clst <= 1 || clst >= n_fatent) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                    fp.clust = clst;
                }
                fp.fptr += ofs;
                if (ofs % FF_MAX_SS != 0) {
                    nsect = clst2sect(clst);
                    if (nsect == 0) { fp.err = FResult.FR_INT_ERR.getValue(); return FResult.FR_INT_ERR; }
                    nsect += ofs / FF_MAX_SS;
                }
            }
        }
        if (fp.fptr > fp.objsize) { fp.objsize = fp.fptr; fp.flag |= FA_MODIFIED; }
        if (fp.fptr % FF_MAX_SS != 0 && nsect != fp.sect) {
            if ((fp.flag & FA_DIRTY) != 0) {
                if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
                fp.flag &= ~FA_DIRTY;
            }
            if (diskIo.diskRead(nsect, fp.buf, 1) != DResult.RES_OK) { fp.err = FResult.FR_DISK_ERR.getValue(); return FResult.FR_DISK_ERR; }
            fp.sect = nsect;
        }
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Truncate File                                                    */
    /*-----------------------------------------------------------------------*/
    public FResult truncate(Fil fp) {
        FResult res = validate(fp);
        if (res != FResult.FR_OK) return res;
        if (fp.err != 0) return FResult.values()[fp.err];
        if ((fp.flag & FA_WRITE) == 0) return FResult.FR_DENIED;
        if (fp.fptr < fp.objsize) {
            if (fp.fptr == 0) {
                res = remove_chain(fp.sclust, 0);
                fp.sclust = 0;
            } else {
                long ncl = get_fat(fp.clust);
                res = FResult.FR_OK;
                if (ncl == 0xFFFFFFFFL) res = FResult.FR_DISK_ERR;
                if (ncl == 1) res = FResult.FR_INT_ERR;
                if (res == FResult.FR_OK && ncl < n_fatent) res = remove_chain(ncl, fp.clust);
            }
            fp.objsize = fp.fptr;
            fp.flag |= FA_MODIFIED;
            if ((fp.flag & FA_DIRTY) != 0) {
                if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) res = FResult.FR_DISK_ERR;
                else fp.flag &= ~FA_DIRTY;
            }
            if (res != FResult.FR_OK) { fp.err = res.getValue(); return res; }
        }
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Synchronize the File                                             */
    /*-----------------------------------------------------------------------*/
    public FResult sync(Fil fp) {
        FResult res = validate(fp);
        if (res != FResult.FR_OK) return res;
        if ((fp.flag & FA_MODIFIED) != 0) {
            if ((fp.flag & FA_DIRTY) != 0) {
                if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR;
                fp.flag &= ~FA_DIRTY;
            }
            res = move_window(fp.dir_sect);
            if (res != FResult.FR_OK) return res;
            win[fp.dir_ptr + DIR_Attr] |= AM_ARC;
            stClustWin(fp.dir_ptr, fp.sclust);
            st_32(win, fp.dir_ptr + DIR_FileSize, fp.objsize);
            st_32(win, fp.dir_ptr + DIR_ModTime, get_fattime());
            // Invalidate LstAccDate
            st_16(win, fp.dir_ptr + DIR_LstAccDate, 0);
            wflag = 1;
            res = sync_fs();
            if (res == FResult.FR_OK) fp.flag &= ~FA_MODIFIED;
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Create a Directory Object                                        */
    /*-----------------------------------------------------------------------*/
    public FResult opendir(Dir dp, String path) {
        if (dp == null) return FResult.FR_INVALID_OBJECT;
        String[] _strippedArr = new String[]{path};

        int _strippedVol = get_ldnumber(_strippedArr);

        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String stripped = _strippedArr[0];
        FResult res = mount_volume(0);
        if (res != FResult.FR_OK) { dp.fs = null; return res; }
        dp.fs = this;
        res = follow_path(dp, stripped);
        if (res == FResult.FR_OK) {
            if ((dp.fn[NSFLAG] & NS_NONAME) == 0) {
                // Check that found object is directory
                res = move_window(dp.sect);
                if (res != FResult.FR_OK) { dp.fs = null; return res; }
                int attr = win[dp.dir_ptr + DIR_Attr] & 0xFF;
                if ((attr & AM_DIR) == 0) { dp.fs = null; return FResult.FR_NO_PATH; }
                dp.sclust = ldClustFromWin(dp.dir_ptr);
            } else {
                dp.sclust = 0;
            }
            dp.id = id;
            res = dir_sdi(dp, 0);
            if (res != FResult.FR_OK) dp.fs = null;
        } else {
            if (res == FResult.FR_NO_FILE) res = FResult.FR_NO_PATH;
            dp.fs = null;
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Close Directory                                                  */
    /*-----------------------------------------------------------------------*/
    public FResult closedir(Dir dp) {
        FResult res = validate(dp);
        if (res != FResult.FR_OK) return res;
        dp.fs = null;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Read Directory Entries in Sequence                               */
    /*-----------------------------------------------------------------------*/
    public FResult readdir(Dir dp, FilInfo fno) {
        FResult res = validate(dp);
        if (res != FResult.FR_OK) return res;
        if (fno == null) {
            // rewind
            return dir_sdi(dp, 0);
        }
        fno.fname = "";
        res = dir_read(dp, 0);
        if (res == FResult.FR_OK) {
            get_fileinfo(dp, fno);
            res = dir_next(dp, false);
            if (res == FResult.FR_NO_FILE) res = FResult.FR_OK;
        }
        if (res == FResult.FR_NO_FILE) {
            fno.fname = "";
            res = FResult.FR_OK;
            dp.sect = 0;
        }
        if (res != FResult.FR_OK) fno.fname = "";
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Get File Status                                                  */
    /*-----------------------------------------------------------------------*/
    public FResult stat(String path, FilInfo fno) {
        String[] _strippedArr = new String[]{path};

        int _strippedVol = get_ldnumber(_strippedArr);

        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String stripped = _strippedArr[0];
        FResult res = mount_volume(0);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = follow_path(dj, stripped);
        if (res == FResult.FR_OK) {
            if ((dj.fn[NSFLAG] & NS_NONAME) != 0) return FResult.FR_INVALID_NAME;
            if (fno != null) {
                res = move_window(dj.sect);
                if (res != FResult.FR_OK) return res;
                // Temporarily set dj dir_ptr correct
                get_fileinfo(dj, fno);
            }
        }
        if (fno != null && res != FResult.FR_OK) fno.fname = "";
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Get Number of Free Clusters                                      */
    /*-----------------------------------------------------------------------*/
    public FResult getfree(String path, long[] nclst) {
        String[] _strippedArr = new String[]{path};

        int _strippedVol = get_ldnumber(_strippedArr);

        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String stripped = _strippedArr[0];
        FResult res = mount_volume(0);
        if (res != FResult.FR_OK) return res;
        if (free_clst <= n_fatent - 2) {
            nclst[0] = free_clst;
            return FResult.FR_OK;
        }
        long nfree = 0;
        if (fs_type == FS_FAT12) {
            long clst = 2;
            for (; clst < n_fatent; clst++) {
                long stat = get_fat(clst);
                if (stat == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                if (stat == 1) return FResult.FR_INT_ERR;
                if (stat == 0) nfree++;
            }
        } else {
            // For FAT16/32 scan FAT entries directly via window for speed
            long clst = n_fatent;
            long sect = fatbase;
            int off = 0;
            // Use move_window scanning per sector
            // Simplify: iterate clusters using get_fat (slower but okay for small images)
            for (long c = 2; c < n_fatent; c++) {
                long stat = get_fat(c);
                if (stat == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                if (stat == 1) return FResult.FR_INT_ERR;
                if (stat == 0) nfree++;
            }
        }
        nclst[0] = nfree;
        free_clst = nfree;
        fsi_flag |= 1;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Get Volume Label                                                 */
    /*-----------------------------------------------------------------------*/
    public FResult getLabel(String path, String[] label, long[] vsn) {
        String[] _pathArr = new String[]{path};
        int vol = get_ldnumber(_pathArr);
        if (vol < 0) return FResult.FR_INVALID_DRIVE;
        FResult res = mount_volume(0);
        if (res != FResult.FR_OK) return res;

        /* Get volume label */
        if (res == FResult.FR_OK && label != null && label.length > 0) {
            Dir dj = new Dir();
            dj.fs = this;
            dj.sclust = 0; /* Open root directory */
            res = dir_sdi(dj, 0);
            if (res == FResult.FR_OK) {
                res = dir_read(dj, 1); /* Find a volume label entry (DIR_READ_LABEL) */
                if (res == FResult.FR_OK) {
                    /* Extract volume label from AM_VOL entry */
                    /* On the FAT/FAT32 volume */
                    StringBuilder sb = new StringBuilder();
                    for (int si = 0; si < 11; si++) {
                        int wc = win[dj.dir_ptr + si] & 0xFF;
                        sb.append((char) wc);
                    }
                    /* Truncate trailing spaces */
                    String s = sb.toString();
                    int di = s.length();
                    while (di > 0 && s.charAt(di - 1) == ' ') di--;
                    s = s.substring(0, di);
                    label[0] = s;
                }
            }
            if (res == FResult.FR_NO_FILE) { /* No label entry and return nul string */
                label[0] = "";
                res = FResult.FR_OK;
            }
        }

        /* Get volume serial number */
        if (res == FResult.FR_OK && vsn != null && vsn.length > 0) {
            res = move_window(volbase); /* Load VBR */
            if (res == FResult.FR_OK) {
                int di;
                /* FF_FS_EXFAT is disabled, so exFAT case is pruned */
                if (fs_type == FS_FAT32) {
                    di = BS_VolID32;
                } else { /* FAT12/16 */
                    di = (win[BS_BootSig] & 0xFF) == 0x29 ? BS_VolID : 0;
                }
                vsn[0] = (di != 0) ? (ld_32(win, di) & 0xFFFFFFFFL) : 0; /* Get VSN in the VBR */
            }
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Delete a File/Directory                                          */
    /*-----------------------------------------------------------------------*/
    public FResult unlink(String path) {
        String[] _strippedArr = new String[]{path};

        int _strippedVol = get_ldnumber(_strippedArr);

        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String stripped = _strippedArr[0];
        FResult res = mount_volume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = follow_path(dj, stripped);
        if (res != FResult.FR_OK) return res;
        if ((dj.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
        res = move_window(dj.sect);
        if (res != FResult.FR_OK) return res;
        int attr = win[dj.dir_ptr + DIR_Attr] & 0xFF;
        if ((attr & AM_RDO) != 0) return FResult.FR_DENIED;
        long dclst = ldClustFromWin(dj.dir_ptr);
        if ((attr & AM_DIR) != 0) {
            Dir sdj = new Dir();
            sdj.fs = this;
            sdj.sclust = dclst;
            res = dir_sdi(sdj, 0);
            if (res != FResult.FR_OK) return res;
            // Check if directory is empty
            while (true) {
                res = move_window(sdj.sect);
                if (res != FResult.FR_OK) return res;
                int et = win[sdj.dir_ptr] & 0xFF;
                if (et == 0) break; // end, empty
                int a = win[sdj.dir_ptr + DIR_Attr] & 0xFF;
                if (et != DDEM && a != AM_LFN) {
                    // Found an entry not deleted
                    // Need to ensure it's not "." or ".." ? For FAT, dot entries exist but they have names "." and ".."
                    // Our dir scan includes them but they are valid entries; we should skip dots for emptiness check
                    // Simple: if entry name is "." or ".." skip
                    boolean isDot = false;
                    if (win[sdj.dir_ptr] == '.' ) {
                        if (win[sdj.dir_ptr+1] == ' ' ) isDot = true;
                        if (win[sdj.dir_ptr+1] == '.' && win[sdj.dir_ptr+2] == ' ') isDot = true;
                    }
                    if (!isDot) return FResult.FR_DENIED;
                }
                res = dir_next(sdj, false);
                if (res == FResult.FR_NO_FILE) break;
                if (res != FResult.FR_OK) return res;
            }
            res = FResult.FR_OK;
        }
        if (res == FResult.FR_OK) {
            // Mark entry deleted
            res = move_window(dj.sect);
            if (res != FResult.FR_OK) return res;
            res = dir_remove(dj);
            if (dclst != 0) {
                res = remove_chain(dclst, 0);
                if (res != FResult.FR_OK) return res;
            }
            res = sync_fs();
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Create a Directory                                               */
    /*-----------------------------------------------------------------------*/
    public FResult mkdir(String path) {
        String[] _strippedArr = new String[]{path};
        int _strippedVol = get_ldnumber(_strippedArr);
        if (_strippedVol < 0) return FResult.FR_INVALID_DRIVE;
        String stripped = _strippedArr[0];
        FResult res = mount_volume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = follow_path(dj, stripped);
        if (res == FResult.FR_OK) {
            if ((dj.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
            return FResult.FR_EXIST;
        }
        if (res != FResult.FR_NO_FILE) return res;
        // It is clear to create a new directory
        // Allocate a cluster for the new directory (single volume, use this as obj)
        long dcl = create_chain(0);
        res = FResult.FR_OK;
        if (dcl == 0) res = FResult.FR_DENIED;
        if (dcl == 1) res = FResult.FR_INT_ERR;
        if (dcl == 0xFFFFFFFFL) res = FResult.FR_DISK_ERR;
        long tm = get_fattime();
        if (res == FResult.FR_OK) {
            res = dir_clear(dcl);
            if (res == FResult.FR_OK) {
                // Create dot entries (FAT only) - in win
                for (int i = 0; i < 11; i++) win[DIR_Name + i] = (byte) ' ';
                win[DIR_Name] = (byte) '.';
                win[DIR_Attr] = (byte) AM_DIR;
                st_32(win, DIR_ModTime, tm);
                st_32(win, DIR_CrtTime, tm);
                stClust(win, 0, dcl);
                // Create ".." entry
                System.arraycopy(win, 0, win, SZDIRE, SZDIRE);
                win[SZDIRE + 1] = (byte) '.';
                long pcl = dj.sclust;
                stClust(win, SZDIRE, pcl);
                wflag = 1;
                res = dir_register(dj);
            }
        }
        if (res == FResult.FR_OK) {
            st_32(win, dj.dir_ptr + DIR_CrtTime, tm);
            st_32(win, dj.dir_ptr + DIR_ModTime, tm);
            stClust(win, dj.dir_ptr, dcl);
            win[dj.dir_ptr + DIR_Attr] = (byte) AM_DIR;
            wflag = 1;
            res = sync_fs();
        } else {
            remove_chain(dcl, 0);
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Rename a File/Directory                                          */
    /*-----------------------------------------------------------------------*/
    public FResult rename(String oldPath, String newPath) {
        String[] _oldStrippedArr = new String[]{oldPath};

        int _oldStrippedVol = get_ldnumber(_oldStrippedArr);

        if (_oldStrippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String oldStripped = _oldStrippedArr[0];
        String[] _newStrippedArr = new String[]{newPath};

        int _newStrippedVol = get_ldnumber(_newStrippedArr);

        if (_newStrippedVol < 0) return FResult.FR_INVALID_DRIVE;

        String newStripped = _newStrippedArr[0];
        FResult res = mount_volume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir djo = new Dir();
        djo.fs = this;
        res = follow_path(djo, oldStripped);
        if (res != FResult.FR_OK) return res;
        if ((djo.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
        res = move_window(djo.sect);
        if (res != FResult.FR_OK) return res;
        byte[] buf = new byte[SZDIRE];
        System.arraycopy(win, djo.dir_ptr, buf, 0, SZDIRE);
        int oldAttr = buf[DIR_Attr] & 0xFF;
        Dir djn = new Dir();
        djn.fs = this;
        // Need to copy parent info for new path: follow_path will set djn to parent of new name
        // But we need to duplicate djo for attribute
        res = follow_path(djn, newStripped);
        if (res == FResult.FR_OK) {
            // Check if same object (same sect+ptr and same cluster)
            if (djn.sect == djo.sect && djn.dir_ptr == djo.dir_ptr) res = FResult.FR_NO_FILE;
            else res = FResult.FR_EXIST;
        }
        if (res != FResult.FR_NO_FILE) {
            if (res == FResult.FR_OK) res = FResult.FR_EXIST;
            return res;
        }
        res = dir_register(djn);
        if (res != FResult.FR_OK) return res;
        // dir_register created SFN entry; now copy other fields from old entry (except name)
        res = move_window(djn.sect);
        if (res != FResult.FR_OK) return res;
        // Copy other fields from buf (except name)
        // Copy attr, times, cluster, size
        win[djn.dir_ptr + DIR_Attr] = buf[DIR_Attr];
        if ((buf[DIR_Attr] & AM_DIR) == 0) win[djn.dir_ptr + DIR_Attr] |= AM_ARC;
        // Copy timestamps etc. Simplified: copy whole 13..32 except name but ensure cluster/size preserved
        System.arraycopy(buf, 13, win, djn.dir_ptr + 13, SZDIRE - 13);
        // For directory being moved, need to update ".." entry if it's a directory
        if ((buf[DIR_Attr] & AM_DIR) != 0 && djo.sclust != djn.sclust) {
            long newCl = ldClust(buf, 0);
            long sec = clst2sect(newCl);
            if (sec == 0) return FResult.FR_INT_ERR;
            res = move_window(sec);
            if (res != FResult.FR_OK) return res;
            // ".." is second entry
            int dotDotPtr = SZDIRE;
            if ((win[dotDotPtr] & 0xFF) == '.' && (win[dotDotPtr+1] & 0xFF) == '.') {
                stClustWin(dotDotPtr, djn.sclust);
                wflag = 1;
                res = sync_window();
                if (res != FResult.FR_OK) return res;
                // Need to bring back window for new entry
                res = move_window(djn.sect);
                if (res != FResult.FR_OK) return res;
            }
        }
        wflag = 1;
        // Remove old entry
        res = dir_remove(djo);
        res = sync_fs();
        return res;
    }

    /* Some API functions are implemented as macro in ff.h */
    //------------------- macro-like helpers -------------------
    public boolean isEof(Fil fp) { return fp.fptr == fp.objsize; }
    public int getError(Fil fp) { return fp.err; }
    public long tell(Fil fp) { return fp.fptr; }
    public long size(Fil fp) { return fp.objsize; }
    public FResult rewind(Fil fp) { return lseek(fp, 0); }
    public FResult rewinddir(Dir dp) { return readdir(dp, null); }
    public FResult rmdir(String path) { return unlink(path); }
    public FResult unmount(String path) { return mount(path, 0); }
}
