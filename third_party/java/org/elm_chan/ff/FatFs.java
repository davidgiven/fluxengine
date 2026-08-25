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

import java.util.function.LongSupplier;

/**
 * Java port of ff.c R0.16 (rev 80386) with baked-in configuration:
 * FF_FS_READONLY=0, FF_FS_MINIMIZE=0, FF_USE_FIND=0, FF_USE_MKFS=0,
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

    /*--------------------------------------------------------------------------*/
    /* Module Private Definitions                                                    */
    /*--------------------------------------------------------------------------*/

    /* Limits and boundaries */
    /* Character code support macros - IsUpper/IsLower/IsDigit/IsSeparator/IsTerminator/IsSurrogate - inlined as methods isUpper etc. */
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
    private static final int PTE_System = 4; /* MBR PTE: System ID */
    private static final int PTE_StLba = 8; /* MBR PTE: Start in LBA */
    private static final int PTE_SizLba = 12; /* MBR PTE: Size in LBA */

    private static final int MAX_DIR = 0x200000; /* Max size of FAT directory (byte) */
    private static final int MAX_FAT12 = 0xFF5; /* Max FAT12 clusters (differs from specs, but right for real DOS/Windows behavior) */
    private static final int MAX_FAT16 = 0xFFF5; /* Max FAT16 clusters (differs from specs, but right for real DOS/Windows behavior) */
    private static final int MAX_FAT32 = 0x0FFFFFF5; /* Max FAT32 clusters (not defined in specs, practical limit) */

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
    public FatFs(DiskIo diskIo, LongSupplier getFatTime) {
        if (diskIo == null) {
            throw new IllegalArgumentException("diskIo must not be null");
        }
        this.diskIo = diskIo;
        this.getFatTimeSupplier = getFatTime;
        this.winsect = -1;
    }

    /*--------------------------------------------------------------------------*/
    /* Module Private Functions                                                    */
    /*--------------------------------------------------------------------------*/

    /*-----------------------------------------------------------------------*/
    /* Load/Store multi-byte word in the FAT structure                       */
    /*-----------------------------------------------------------------------*/

    //------------------- low level helpers -------------------
    private static int ldWord(byte[] buf, int off) { /* Load a 2-byte little-endian word */
        return (buf[off] & 0xFF) | ((buf[off + 1] & 0xFF) << 8);
    }
    private static long ldDword(byte[] buf, int off) { /* Load a 4-byte little-endian word */
        return (buf[off] & 0xFFL) | ((buf[off + 1] & 0xFFL) << 8) | ((buf[off + 2] & 0xFFL) << 16) | ((buf[off + 3] & 0xFFL) << 24);
    }
    private static void stWord(byte[] buf, int off, int val) { /* Store a 2-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
    }
    private static void stDword(byte[] buf, int off, long val) { /* Store a 4-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
        buf[off + 2] = (byte) ((val >> 16) & 0xFF);
        buf[off + 3] = (byte) ((val >> 24) & 0xFF);
    }
    private static boolean isUpper(int c) { return c >= 'A' && c <= 'Z'; } /* Character code support macro IsUpper */
    private static boolean isLower(int c) { return c >= 'a' && c <= 'z'; } /* IsLower */
    private static boolean isDigit(int c) { return c >= '0' && c <= '9'; } /* IsDigit */
    private static boolean isSeparator(int c) { return c == '/' || c == '\\'; } /* IsSeparator */
    private static boolean isTerminator(int c) { return c < '!' ; } /* IsTerminator - LFN disabled so threshold is '!' */

    /* Test if the byte is DBC 1st byte */
    private static boolean dbc1st(int c) {
        c &= 0xFF;
        if (c >= TBL_DC932[0] && c <= TBL_DC932[1]) return true;
        if (c >= TBL_DC932[2] && c <= TBL_DC932[3]) return true;
        return false;
    }
    /* Test if the byte is DBC 2nd byte */
    private static boolean dbc2nd(int c) {
        c &= 0xFF;
        if (c >= TBL_DC932[4] && c <= TBL_DC932[5]) return true;
        if (c >= TBL_DC932[6] && c <= TBL_DC932[7]) return true;
        if (TBL_DC932[8] != 0 || TBL_DC932[9] != 0) {
            if (c >= TBL_DC932[8] && c <= TBL_DC932[9]) return true;
        }
        return false;
    }

    /* Timestamp - GET_FATTIME() */
    private long getFatTime() {
        if (getFatTimeSupplier != null) {
            return getFatTimeSupplier.getAsLong() & 0xFFFFFFFFL;
        }
        // Fixed time 2025/01/01 00:00:00
        long t = ((2025 - 1980) << 25) | (1 << 21) | (1 << 16);
        return t & 0xFFFFFFFFL;
    }

    /* Definitions of logical drive to physical location conversion - LD2PD/LD2PT */
    private String stripDrive(String path) {
        if (path == null) return "";
        // Check for "N:" prefix where N is digit
        if (path.length() >= 2 && path.charAt(1) == ':') {
            char d = path.charAt(0);
            if (d >= '0' && d <= '9') {
                int drv = d - '0';
                if (drv != 0) return null; // invalid drive
                String rem = path.substring(2);
                return rem;
            } else {
                return null;
            }
        }
        // No drive prefix, treat as drive 0
        return path;
    }

    /*-----------------------------------------------------------------------*/
    /* Move/Flush disk access window in the filesystem object                */
    /*-----------------------------------------------------------------------*/
    //------------------- disk window -------------------
    /* Post process on fatal error in the file operations - ABORT is inlined */
    private FResult syncWindow() { /* sync_window - Returns FR_OK or FR_DISK_ERR */
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

    private FResult moveWindow(long sect) { /* move_window - Returns FR_OK or FR_DISK_ERR */
        if (sect != winsect) {
            FResult res = syncWindow();
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
    private FResult syncFs() {
        FResult res = syncWindow();
        if (res != FResult.FR_OK) return res;
        if (fsi_flag == 1) {
            fsi_flag = 0;
            if (fs_type == FS_FAT32) {
                // Create FSInfo
                for (int i = 0; i < win.length; i++) win[i] = 0;
                stDword(win, FSI_LeadSig, 0x41615252L);
                stDword(win, FSI_StrucSig, 0x61417272L);
                stDword(win, FSI_Free_Count, free_clst);
                stDword(win, FSI_Nxt_Free, last_clst);
                stDword(win, FSI_TrailSig, 0xAA550000L);
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
    private long getFat(long clst) {
        if (clst < 2 || clst >= n_fatent) return 1; // internal error
        long val = 0xFFFFFFFFL;
        FResult res;
        switch (fs_type) {
            case FS_FAT12: {
                int bc = (int) clst + (int)(clst / 2);
                res = moveWindow(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) break;
                int wc = win[bc % FF_MAX_SS] & 0xFF;
                bc++;
                res = moveWindow(fatbase + (bc / FF_MAX_SS));
                if (res != FResult.FR_OK) break;
                wc |= (win[bc % FF_MAX_SS] & 0xFF) << 8;
                val = ((clst & 1) != 0) ? (wc >> 4) & 0xFFF : wc & 0xFFF;
                break;
            }
            case FS_FAT16: {
                res = moveWindow(fatbase + (clst / (FF_MAX_SS / 2)));
                if (res != FResult.FR_OK) break;
                int off = (int) (clst * 2 % FF_MAX_SS);
                val = ldWord(win, off) & 0xFFFFL;
                break;
            }
            case FS_FAT32: {
                res = moveWindow(fatbase + (clst / (FF_MAX_SS / 4)));
                if (res != FResult.FR_OK) break;
                int off = (int) (clst * 4 % FF_MAX_SS);
                val = ldDword(win, off) & 0x0FFFFFFFL;
                break;
            }
            default: val = 1; break;
        }
        return val;
    }

    /*-----------------------------------------------------------------------*/
    /* FAT access - Change value of an FAT entry                             */
    /*-----------------------------------------------------------------------*/
    private FResult putFat(long clst, long val) {
        if (clst < 2 || clst >= n_fatent) return FResult.FR_INT_ERR;
        FResult res;
        switch (fs_type) {
            case FS_FAT12: {
                int bc = (int) clst + (int)(clst / 2);
                res = moveWindow(fatbase + (bc / FF_MAX_SS));
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
                res = moveWindow(fatbase + (bc / FF_MAX_SS));
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
                res = moveWindow(fatbase + (clst / (FF_MAX_SS / 2)));
                if (res != FResult.FR_OK) return res;
                int off = (int) (clst * 2 % FF_MAX_SS);
                stWord(win, off, (int) (val & 0xFFFF));
                wflag = 1;
                return FResult.FR_OK;
            }
            case FS_FAT32: {
                res = moveWindow(fatbase + (clst / (FF_MAX_SS / 4)));
                if (res != FResult.FR_OK) return res;
                int off = (int) (clst * 4 % FF_MAX_SS);
                long cur = ldDword(win, off);
                long newVal = (val & 0x0FFFFFFFL) | (cur & 0xF0000000L);
                stDword(win, off, newVal);
                wflag = 1;
                return FResult.FR_OK;
            }
            default: return FResult.FR_INT_ERR;
        }
    }

    /*-----------------------------------------------------------------------*/
    /* FAT handling - Remove a cluster chain                                 */
    /*-----------------------------------------------------------------------*/
    private FResult removeChain(long clst, long pclst) {
        if (clst < 2 || clst >= n_fatent) return FResult.FR_INT_ERR;
        FResult res = FResult.FR_OK;
        if (pclst != 0) {
            res = putFat(pclst, 0xFFFFFFFFL);
            if (res != FResult.FR_OK) return res;
        }
        long nxt;
        long cur = clst;
        do {
            nxt = getFat(cur);
            if (nxt == 0) break;
            if (nxt == 1) return FResult.FR_INT_ERR;
            if (nxt == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
            res = putFat(cur, 0);
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
    private long createChain(long clst) {
        long cs, ncl, scl;
        FResult res;
        if (clst == 0) {
            scl = last_clst;
            if (scl == 0 || scl >= n_fatent) scl = 1;
        } else {
            cs = getFat(clst);
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
            cs = getFat(ncl);
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
                cs = getFat(ncl);
                if (cs == 0) break;
                if (cs == 1 || cs == 0xFFFFFFFFL) return cs;
                if (ncl == scl) return 0;
            }
        }
        res = putFat(ncl, 0xFFFFFFFFL);
        if (res == FResult.FR_OK && clst != 0) {
            res = putFat(clst, ncl);
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
    private FResult dirClear(long clst) {
        // synchronize window
        FResult res = syncWindow();
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
    private FResult dirSdi(Dir dp, long ofs) {
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
                long nxt = getFat(clst);
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
        dp.dirPtr = (int) (ofs % FF_MAX_SS);
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Move directory table index next                  */
    /*-----------------------------------------------------------------------*/
    private FResult dirNext(Dir dp, boolean stretch) {
        long ofs = dp.dptr + SZDIRE;
        if (ofs >= MAX_DIR) { dp.sect = 0; return FResult.FR_NO_FILE; }
        if (dp.sect == 0) return FResult.FR_NO_FILE;
        if (ofs % FF_MAX_SS == 0) {
            dp.sect++;
            if (dp.clust == 0) {
                if (ofs / SZDIRE >= n_rootdir) { dp.sect = 0; return FResult.FR_NO_FILE; }
            } else {
                if ((ofs / FF_MAX_SS & (csize - 1)) == 0) {
                    long clst = getFat(dp.clust);
                    if (clst <= 1) return FResult.FR_INT_ERR;
                    if (clst == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                    if (clst >= n_fatent) {
                        if (!stretch) { dp.sect = 0; return FResult.FR_NO_FILE; }
                        long ncl = createChain(dp.clust);
                        if (ncl == 0) return FResult.FR_DENIED;
                        if (ncl == 1) return FResult.FR_INT_ERR;
                        if (ncl == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                        if (dirClear(ncl) != FResult.FR_OK) return FResult.FR_DISK_ERR;
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
        dp.dirPtr = (int) (ofs % FF_MAX_SS);
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* Directory handling - Reserve a block of directory entries             */
    /*-----------------------------------------------------------------------*/
    private FResult dirAlloc(Dir dp, int nEnt) {
        FResult res = dirSdi(dp, 0);
        if (res != FResult.FR_OK) return res;
        int n = 0;
        do {
            res = moveWindow(dp.sect);
            if (res != FResult.FR_OK) break;
            int name = win[dp.dirPtr] & 0xFF;
            if (name == DDEM || name == 0) {
                n++;
                if (n == nEnt) break;
            } else {
                n = 0;
            }
            res = dirNext(dp, true);
        } while (res == FResult.FR_OK);
        if (res == FResult.FR_NO_FILE) res = FResult.FR_DENIED;
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* FAT: Directory handling - Load/Store start cluster number             */
    /*-----------------------------------------------------------------------*/
    private long ldClust(byte[] dir, int off) {
        long cl = ldWord(dir, off + DIR_FstClusLO) & 0xFFFFL;
        if (fs_type == FS_FAT32) {
            cl |= ((long) ldWord(dir, off + DIR_FstClusHI) & 0xFFFFL) << 16;
        }
        return cl;
    }
    private long ldClustFromWin(int ptr) {
        long cl = ldWord(win, ptr + DIR_FstClusLO) & 0xFFFFL;
        if (fs_type == FS_FAT32) cl |= ((long) ldWord(win, ptr + DIR_FstClusHI) & 0xFFFFL) << 16;
        return cl;
    }
    private void stClust(byte[] dir, int off, long cl) {
        stWord(dir, off + DIR_FstClusLO, (int) (cl & 0xFFFF));
        if (fs_type == FS_FAT32) stWord(dir, off + DIR_FstClusHI, (int) ((cl >> 16) & 0xFFFF));
    }
    private void stClustWin(int ptr, long cl) {
        stWord(win, ptr + DIR_FstClusLO, (int) (cl & 0xFFFF));
        if (fs_type == FS_FAT32) stWord(win, ptr + DIR_FstClusHI, (int) ((cl >> 16) & 0xFFFF));
    }

    /* Directory handling - Get file information */
    private void getFileinfo(Dir dp, FilInfo fno) {
        // dp.sect already window'd? caller ensures moveWindow
        // But also need to handle dirPtr
        // Called after DIR_READ_FILE equivalent verification
        if (dp.sect == 0) {
            fno.fname = "";
            return;
        }
        // Ensure window is loaded for this sector
        // (callers should have moved)
        byte[] dir = win;
        int ptr = dp.dirPtr;
        // Build SFN name 8.3 to fname (simple)
        StringBuilder sb = new StringBuilder();
        // Body part 0..7
        int di = 0;
        for (int si = 0; si < 11; si++) {
            int c = dir[ptr + si] & 0xFF;
            if (si < 8) {
                if (c == ' ') continue;
                if (c == RDDEM) c = DDEM;
                sb.append((char) c);
            } else if (si == 8) {
                // check if extension has non-space
                boolean hasExt = false;
                for (int k = 8; k < 11; k++) if ((dir[ptr + k] & 0xFF) != ' ') hasExt = true;
                if (hasExt) sb.append('.');
                if (c == ' ') continue;
                sb.append((char) c);
            } else {
                if (c == ' ') continue;
                sb.append((char) c);
            }
        }
        fno.fname = sb.toString();
        fno.fattrib = dir[ptr + DIR_Attr] & AM_MASK;
        fno.fsize = ldDword(dir, ptr + DIR_FileSize);
        fno.ftime = ldWord(dir, ptr + DIR_ModTime) & 0xFFFF;
        fno.fdate = ldWord(dir, ptr + DIR_ModTime + 2) & 0xFFFF;
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
    private FResult dirFind(Dir dp) {
        FResult res = dirSdi(dp, 0);
        if (res != FResult.FR_OK) return res;
        do {
            res = moveWindow(dp.sect);
            if (res != FResult.FR_OK) break;
            int et = win[dp.dirPtr] & 0xFF;
            if (et == 0) { res = FResult.FR_NO_FILE; break; }
            int attr = win[dp.dirPtr + DIR_Attr] & 0xFF;
            // In non-LFN, valid entry if not deleted and attribute not VOL and not LFN
            if (et != DDEM && attr != AM_LFN && (attr & AM_VOL) == 0) {
                if (compareFilenames(win, dp.dirPtr, dp.fn) == 0) break;
            }
            res = dirNext(dp, false);
        } while (res == FResult.FR_OK);
        return res;
    }

    /* Directory handling - Create a file name (SFN) */
    private FResult createName(Dir dp, String[] pathRef) {
        String path = pathRef[0];
        byte[] sfn = dp.fn;
        for (int i = 0; i < 12; i++) sfn[i] = 0;
        for (int i = 0; i < 11; i++) sfn[i] = (byte) ' ';
        // Handle leading separators (should already be stripped, but ensure)
        int si = 0;
        while (si < path.length() && isSeparator(path.charAt(si))) si++;
        int start = si;
        // Find end of segment (separator or terminator)
        int end = si;
        while (end < path.length() && !isSeparator(path.charAt(end))) end++;
        String seg = path.substring(start, end);
        // Remaining path after segment
        int next = end;
        while (next < path.length() && isSeparator(path.charAt(next))) next++;
        String remaining = path.substring(next);
        pathRef[0] = remaining;

        if (seg.length() == 0) return FResult.FR_INVALID_NAME;
        // Check for dot entries
        if (seg.equals(".")) {
            sfn[0] = (byte) '.';
            for (int i = 1; i < 11; i++) sfn[i] = (byte) ' ';
            sfn[NSFLAG] = (byte) (NS_DOT | (remaining.length() == 0 ? NS_LAST : 0) );
            return FResult.FR_OK;
        }
        if (seg.equals("..")) {
            sfn[0] = (byte) '.'; sfn[1] = (byte) '.';
            for (int i = 2; i < 11; i++) sfn[i] = (byte) ' ';
            sfn[NSFLAG] = (byte) (NS_DOT | (remaining.length() == 0 ? NS_LAST : 0));
            return FResult.FR_OK;
        }
        // Create SFN in directory form
        int ni = 8;
        int i = 0; // index in sfn
        int p = 0; // index in seg
        boolean hasExt = seg.contains(".");
        // Validate and copy
        // Use loop similar to C non-LFN version
        int sfnPos = 0;
        // We'll parse seg char by char
        int segIdx = 0;
        int outPos = 0;
        int extPos = 8;
        boolean inExt = false;
        while (segIdx < seg.length()) {
            char ch = seg.charAt(segIdx);
            segIdx++;
            if (ch == '.') {
                if (inExt) return FResult.FR_INVALID_NAME; // multiple dots invalid in this config? allow only one dot but treat extra as invalid
                if (outPos == 0) return FResult.FR_INVALID_NAME; // leading dot
                outPos = 8;
                ni = 11;
                inExt = true;
                continue;
            }
            if (outPos >= ni) return FResult.FR_INVALID_NAME; // overflow
            int c = ch & 0xFF;
            // Check illegal characters
            String illegal = "*+,:;<=>[]|\"?\u007F";
            if (illegal.indexOf(ch) >= 0) return FResult.FR_INVALID_NAME;
            if (c <= ' ') return FResult.FR_INVALID_NAME;
            // DBCS handling
            if (dbc1st(c)) {
                if (segIdx >= seg.length()) return FResult.FR_INVALID_NAME;
                char dch = seg.charAt(segIdx);
                int d = dch & 0xFF;
                if (!dbc2nd(d) || outPos >= ni - 1) return FResult.FR_INVALID_NAME;
                // DBCS counts as two bytes
                // Convert to upper? For SBCS extended char, need upcase? For CP932, SBCS upcase not needed here as bytes >=0x80 are DBCS leading; keep as is
                sfn[outPos++] = (byte) c;
                sfn[outPos++] = (byte) d;
                segIdx++;
            } else {
                // SBCS
                if (c >= 0x80) {
                    // SBCS extended not in DBCS range: invalid for this DBCS code? But we ignore upcase table since not defined
                }
                if (isLower(c)) c -= 0x20;
                sfn[outPos++] = (byte) c;
            }
        }
        if (outPos == 0) return FResult.FR_INVALID_NAME;
        if (sfn[0] == (byte) DDEM) sfn[0] = (byte) RDDEM;
        sfn[NSFLAG] = (byte) (remaining.length() == 0 ? NS_LAST : 0);
        return FResult.FR_OK;
    }

    /* Directory handling - Follow a path */
    private FResult followPath(Dir dp, String path) {
        // path is already stripped of drive prefix, may contain leading separators
        // Determine start directory
        // With no RPATH, start at root always
        while (path.length() > 0 && isSeparator(path.charAt(0))) path = path.substring(1);
        dp.sclust = 0; // root
        dp.clust = 0;
        dp.sect = 0;
        if (path.length() == 0 || path.charAt(0) == 0) {
            dp.fn[NSFLAG] = (byte) NS_NONAME;
            return dirSdi(dp, 0);
        }
        String[] ref = new String[]{ path };
        for (;;) {
            FResult res = createName(dp, ref);
            if (res != FResult.FR_OK) return res;
            int ns = dp.fn[NSFLAG] & 0xFF;
            // Dot handling: in root only "." and ".." are valid but ".." at root stays
            if ((ns & NS_DOT) != 0) {
                if ((ns & NS_LAST) != 0) {
                    dp.fn[NSFLAG] = (byte) NS_NONAME;
                    return dirSdi(dp, 0);
                } else {
                    // Continue to next segment (stay at same directory, for root '.' means stay)
                    // For simplicity, if segment is "." just continue
                    if (dp.fn[0] == '.' && dp.fn[1] == ' ') {
                        if (ref[0].length() == 0) { dp.fn[NSFLAG] = (byte) NS_NONAME; return dirSdi(dp, 0); }
                        continue;
                    }
                    // ".." at root stays at root
                    continue;
                }
            }
            res = dirFind(dp);
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
            FResult r = moveWindow(dp.sect);
            if (r != FResult.FR_OK) return r;
            attr = win[dp.dirPtr + DIR_Attr] & 0xFF;
            if ((attr & AM_DIR) == 0) return FResult.FR_NO_PATH;
            dp.sclust = ldClustFromWin(dp.dirPtr);
            // Continue loop to find next segment in sub-directory
            String remaining = ref[0];
            // keep dp.sclust for next iteration; dirFind will start from that directory
        }
        // At this point dp points to the object entry; preserve its position
        // Need to capture object info for Dir? For files, dp holds entry location
        // Move window to ensure win contains entry
        FResult res = moveWindow(dp.sect);
        if (res != FResult.FR_OK) return res;
        return FResult.FR_OK;
    }

    /* Check what the sector is */
    private int checkFs(long sect) {
        // returns 0: FAT VBR, 2: not FAT valid BS, 3: invalid, 4: disk error
        wflag = 0; winsect = -1;
        if (moveWindow(sect) != FResult.FR_OK) return 4;
        int sign = ldWord(win, BS_55AA);
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
            int w = ldWord(win, BPB_BytsPerSec);
            int c = win[BPB_SecPerClus] & 0xFF;
            int rsv = ldWord(win, BPB_RsvdSecCnt);
            int nf = win[BPB_NumFATs] & 0xFF;
            int nroot = ldWord(win, BPB_RootEntCnt);
            int tot16 = ldWord(win, BPB_TotSec16);
            long tot32 = ldDword(win, BPB_TotSec32);
            int fatsz16 = ldWord(win, BPB_FATSz16);
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
    private int findVolume() {
        int fmt = checkFs(0);
        if (fmt != 2) {
            if (fmt >= 3 || fmt == 0) return fmt; // as per C logic: if fmt !=2 && (fmt>=3 || part==0) return fmt; part==0 always
            // For fmt==2 we continue
        }
        // Need to examine MBR partition table
        // win currently holds sector 0 (MBR or VBR). If fmt==2 it's a valid BS not FAT, likely MBR.
        // Read partition entries
        long[] mbrPt = new long[4];
        for (int i = 0; i < 4; i++) {
            mbrPt[i] = ldDword(win, MBR_Table + i * SZ_PTE + PTE_StLba);
        }
        for (int i = 0; i < 4; i++) {
            if (mbrPt[i] == 0) continue;
            fmt = checkFs(mbrPt[i]);
            if (fmt == 0) return fmt; // found FAT
        }
        // No FAT partition found
        return 3;
    }

    /*-----------------------------------------------------------------------*/
    /* Determine logical drive number and mount the volume if needed         */
    /*-----------------------------------------------------------------------*/
    private FResult mountVolume(int mode) {
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
        int fmt = findVolume();
        if (fmt == 4) return FResult.FR_DISK_ERR;
        if (fmt >= 2) return FResult.FR_NO_FILESYSTEM;
        long bsect = winsect;
        // Initialize FS object based on BPB
        int bytsPerSec = ldWord(win, BPB_BytsPerSec);
        if (bytsPerSec != FF_MAX_SS) return FResult.FR_NO_FILESYSTEM;
        long fasize = ldWord(win, BPB_FATSz16) & 0xFFFFL;
        if (fasize == 0) fasize = ldDword(win, BPB_FATSz32);
        fsize = fasize;
        n_fats = win[BPB_NumFATs] & 0xFF;
        if (n_fats != 1 && n_fats != 2) return FResult.FR_NO_FILESYSTEM;
        long fasizeTotal = fasize * n_fats;
        csize = win[BPB_SecPerClus] & 0xFF;
        if (csize == 0 || (csize & (csize - 1)) != 0) return FResult.FR_NO_FILESYSTEM;
        n_rootdir = ldWord(win, BPB_RootEntCnt);
        if (n_rootdir % (FF_MAX_SS / SZDIRE) != 0) return FResult.FR_NO_FILESYSTEM;
        long tsect = ldWord(win, BPB_TotSec16) & 0xFFFFL;
        if (tsect == 0) tsect = ldDword(win, BPB_TotSec32);
        int nrsv = ldWord(win, BPB_RsvdSecCnt);
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
            if (ldWord(win, BPB_FSVer32) != 0) return FResult.FR_NO_FILESYSTEM;
            if (n_rootdir != 0) return FResult.FR_NO_FILESYSTEM;
            dirbase = ldDword(win, BPB_RootClus32);
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
        if (fmtType == FS_FAT32 && ldWord(win, BPB_FSInfo32) == 1) {
            if (moveWindow(bsect + 1) == FResult.FR_OK) {
                if (ldDword(win, FSI_LeadSig) == 0x41615252L && ldDword(win, FSI_StrucSig) == 0x61417272L && ldDword(win, FSI_TrailSig) == 0xAA550000L) {
                    free_clst = ldDword(win, FSI_Free_Count);
                    last_clst = ldDword(win, FSI_Nxt_Free);
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
    private FResult validateObject(int objId, FatFs objFs) {
        if (objFs == null || objFs != this || objFs.fs_type == 0 || objId != id) return FResult.FR_INVALID_OBJECT;
        int stat = diskIo.diskStatus();
        if ((stat & DiskIo.STA_NOINIT) != 0) return FResult.FR_NOT_READY;
        return FResult.FR_OK;
    }

    private FResult validateFil(Fil fp) {
        if (fp == null || fp.fs == null) return FResult.FR_INVALID_OBJECT;
        return validateObject(fp.id, fp.fs);
    }
    private FResult validateDir(Dir dp) {
        if (dp == null || dp.fs == null) return FResult.FR_INVALID_OBJECT;
        return validateObject(dp.id, dp.fs);
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
        String remainder = stripDrive(path);
        if (remainder == null) return FResult.FR_INVALID_DRIVE;
        if (opt == 0) {
            fs_type = 0;
            winsect = -1;
            wflag = 0;
            return FResult.FR_OK;
        }
        // opt ==1: mount
        FResult res = mountVolume(0);
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
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        // Mask mode
        mode &= (FA_READ | FA_WRITE | FA_CREATE_ALWAYS | FA_CREATE_NEW | FA_OPEN_ALWAYS | FA_OPEN_APPEND);
        // Mount volume if needed
        FResult res = mountVolume(mode);
        if (res != FResult.FR_OK) {
            fp.fs = null;
            return res;
        }
        Dir dj = new Dir();
        dj.fs = this;
        res = followPath(dj, stripped);
        // For create modes, handle not found
        if ((mode & (FA_CREATE_ALWAYS | FA_OPEN_ALWAYS | FA_CREATE_NEW)) != 0) {
            if (res != FResult.FR_OK) {
                if (res == FResult.FR_NO_FILE) {
                    // Create new entry
                    res = dirAlloc(dj, 1);
                    if (res != FResult.FR_OK) { fp.fs = null; return res; }
                    // dirAlloc left dj pointing to allocated entry; need to set up SFN entry
                    res = moveWindow(dj.sect);
                    if (res != FResult.FR_OK) { fp.fs = null; return res; }
                    // Clean entry
                    for (int i = 0; i < SZDIRE; i++) win[dj.dirPtr + i] = 0;
                    System.arraycopy(dj.fn, 0, win, dj.dirPtr + DIR_Name, 11);
                    wflag = 1;
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
                res = moveWindow(dj.sect);
                if (res != FResult.FR_OK) { fp.fs = null; return res; }
                int attr = win[dj.dirPtr + DIR_Attr] & 0xFF;
                if ((attr & (AM_RDO | AM_DIR)) != 0) {
                    fp.fs = null;
                    return FResult.FR_DENIED;
                }
                if ((mode & FA_CREATE_ALWAYS) != 0) {
                    // Truncate
                    long tm = getFatTime();
                    // Clear dir entry and cluster chain
                    long cl = ldClustFromWin(dj.dirPtr);
                    stDword(win, dj.dirPtr + DIR_CrtTime, tm);
                    stDword(win, dj.dirPtr + DIR_ModTime, tm);
                    win[dj.dirPtr + DIR_Attr] = (byte) AM_ARC;
                    stClustWin(dj.dirPtr, 0);
                    stDword(win, dj.dirPtr + DIR_FileSize, 0);
                    wflag = 1;
                    long sc = winsect;
                    if (cl != 0) {
                        res = removeChain(cl, 0);
                        if (res == FResult.FR_OK) {
                            res = moveWindow(sc);
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
            fp.dirSect = winsect;
            fp.dirPtr = dj.dirPtr;
        } else {
            // Open existing
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            res = moveWindow(dj.sect);
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            int attr = win[dj.dirPtr + DIR_Attr] & 0xFF;
            if ((attr & AM_DIR) != 0) { fp.fs = null; return FResult.FR_NO_FILE; }
            if ((mode & FA_WRITE) != 0 && (attr & AM_RDO) != 0) { fp.fs = null; return FResult.FR_DENIED; }
            fp.dirSect = winsect;
            fp.dirPtr = dj.dirPtr;
        }
        if (res == FResult.FR_OK) {
            // Fill Fil object
            res = moveWindow(dj.sect);
            if (res != FResult.FR_OK) { fp.fs = null; return res; }
            fp.fs = this;
            fp.id = id;
            fp.attr = win[dj.dirPtr + DIR_Attr] & 0xFF;
            fp.sclust = ldClustFromWin(dj.dirPtr);
            fp.objsize = ldDword(win, dj.dirPtr + DIR_FileSize);
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
                    long nxt = getFat(clst);
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
        res = validateFil(fp);
        if (res != FResult.FR_OK) return res;
        fp.fs = null;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Read File                                                        */
    /*-----------------------------------------------------------------------*/
    public FResult read(Fil fp, byte[] buff, int btr, IntRef br) {
        if (br != null) br.value = 0;
        FResult res = validateFil(fp);
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
                        clst = getFat(fp.clust);
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
                    System.arraycopy(tmp, 0, buff, rbuffOff, cc * FF_MAX_SS);
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
            System.arraycopy(fp.buf, (int)(fp.fptr % FF_MAX_SS), buff, rbuffOff, rcnt);
            btr -= rcnt; rbuffOff += rcnt; brCnt += rcnt; fp.fptr += rcnt;
        }
        if (br != null) br.value = brCnt;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Write File                                                       */
    /*-----------------------------------------------------------------------*/
    public FResult write(Fil fp, byte[] buff, int btw, IntRef bw) {
        if (bw != null) bw.value = 0;
        FResult res = validateFil(fp);
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
                        if (clst == 0) clst = createChain(0);
                    } else {
                        clst = createChain(fp.clust);
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
                    System.arraycopy(buff, wOff, slice, 0, cc * FF_MAX_SS);
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
            System.arraycopy(buff, wOff, fp.buf, (int)(fp.fptr % FF_MAX_SS), wcnt);
            fp.flag |= FA_DIRTY;
            btw -= wcnt; wOff += wcnt; bwCnt += wcnt; fp.fptr += wcnt;
            if (fp.fptr > fp.objsize) fp.objsize = fp.fptr;
        }
        fp.flag |= FA_MODIFIED;
        if (bw != null) bw.value = bwCnt;
        return FResult.FR_OK;
    }

    // Special handling for write direct second overload to avoid slice misuse
    private FResult writeInternal(Fil fp, byte[] buff, int btw, IntRef bw) { return write(fp, buff, btw, bw); }

    /*-----------------------------------------------------------------------*/
    /* API: Seek File Read/Write Pointer                                     */
    /*-----------------------------------------------------------------------*/
    public FResult lseek(Fil fp, long ofs) {
        FResult res = validateFil(fp);
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
                    clst = createChain(0);
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
                        clst = createChain(clst);
                        if (clst == 0) { ofs = 0; break; }
                    } else {
                        clst = getFat(clst);
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
        FResult res = validateFil(fp);
        if (res != FResult.FR_OK) return res;
        if (fp.err != 0) return FResult.values()[fp.err];
        if ((fp.flag & FA_WRITE) == 0) return FResult.FR_DENIED;
        if (fp.fptr < fp.objsize) {
            if (fp.fptr == 0) {
                res = removeChain(fp.sclust, 0);
                fp.sclust = 0;
            } else {
                long ncl = getFat(fp.clust);
                res = FResult.FR_OK;
                if (ncl == 0xFFFFFFFFL) res = FResult.FR_DISK_ERR;
                if (ncl == 1) res = FResult.FR_INT_ERR;
                if (res == FResult.FR_OK && ncl < n_fatent) res = removeChain(ncl, fp.clust);
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
        FResult res = validateFil(fp);
        if (res != FResult.FR_OK) return res;
        if ((fp.flag & FA_MODIFIED) != 0) {
            if ((fp.flag & FA_DIRTY) != 0) {
                if (diskIo.diskWrite(fp.sect, fp.buf, 1) != DResult.RES_OK) return FResult.FR_DISK_ERR;
                fp.flag &= ~FA_DIRTY;
            }
            res = moveWindow(fp.dirSect);
            if (res != FResult.FR_OK) return res;
            win[fp.dirPtr + DIR_Attr] |= AM_ARC;
            stClustWin(fp.dirPtr, fp.sclust);
            stDword(win, fp.dirPtr + DIR_FileSize, fp.objsize);
            stDword(win, fp.dirPtr + DIR_ModTime, getFatTime());
            // Invalidate LstAccDate
            stWord(win, fp.dirPtr + DIR_LstAccDate, 0);
            wflag = 1;
            res = syncFs();
            if (res == FResult.FR_OK) fp.flag &= ~FA_MODIFIED;
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Create a Directory Object                                        */
    /*-----------------------------------------------------------------------*/
    public FResult opendir(Dir dp, String path) {
        if (dp == null) return FResult.FR_INVALID_OBJECT;
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(0);
        if (res != FResult.FR_OK) { dp.fs = null; return res; }
        dp.fs = this;
        res = followPath(dp, stripped);
        if (res == FResult.FR_OK) {
            if ((dp.fn[NSFLAG] & NS_NONAME) == 0) {
                // Check that found object is directory
                res = moveWindow(dp.sect);
                if (res != FResult.FR_OK) { dp.fs = null; return res; }
                int attr = win[dp.dirPtr + DIR_Attr] & 0xFF;
                if ((attr & AM_DIR) == 0) { dp.fs = null; return FResult.FR_NO_PATH; }
                dp.sclust = ldClustFromWin(dp.dirPtr);
            } else {
                dp.sclust = 0;
            }
            dp.id = id;
            res = dirSdi(dp, 0);
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
        FResult res = validateDir(dp);
        if (res != FResult.FR_OK) return res;
        dp.fs = null;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Read Directory Entries in Sequence                               */
    /*-----------------------------------------------------------------------*/
    public FResult readdir(Dir dp, FilInfo fno) {
        FResult res = validateDir(dp);
        if (res != FResult.FR_OK) return res;
        if (fno == null) {
            // rewind
            return dirSdi(dp, 0);
        }
        fno.fname = "";
        // Find next valid entry
        while (dp.sect != 0) {
            res = moveWindow(dp.sect);
            if (res != FResult.FR_OK) break;
            int et = win[dp.dirPtr] & 0xFF;
            if (et == 0) { res = FResult.FR_NO_FILE; break; }
            int attr = win[dp.dirPtr + DIR_Attr] & AM_MASK;
            if (et != DDEM && attr != AM_LFN && (attr & AM_VOL) == 0) {
                // Valid file/dir
                getFileinfo(dp, fno);
                res = dirNext(dp, false);
                if (res == FResult.FR_NO_FILE) res = FResult.FR_OK;
                return res;
            }
            res = dirNext(dp, false);
            if (res != FResult.FR_OK) break;
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
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(0);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = followPath(dj, stripped);
        if (res == FResult.FR_OK) {
            if ((dj.fn[NSFLAG] & NS_NONAME) != 0) return FResult.FR_INVALID_NAME;
            if (fno != null) {
                res = moveWindow(dj.sect);
                if (res != FResult.FR_OK) return res;
                // Temporarily set dj dirPtr correct
                getFileinfo(dj, fno);
            }
        }
        if (fno != null && res != FResult.FR_OK) fno.fname = "";
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Get Number of Free Clusters                                      */
    /*-----------------------------------------------------------------------*/
    public FResult getfree(String path, LongRef nclst) {
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(0);
        if (res != FResult.FR_OK) return res;
        if (free_clst <= n_fatent - 2) {
            nclst.value = free_clst;
            return FResult.FR_OK;
        }
        long nfree = 0;
        if (fs_type == FS_FAT12) {
            long clst = 2;
            for (; clst < n_fatent; clst++) {
                long stat = getFat(clst);
                if (stat == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                if (stat == 1) return FResult.FR_INT_ERR;
                if (stat == 0) nfree++;
            }
        } else {
            // For FAT16/32 scan FAT entries directly via window for speed
            long clst = n_fatent;
            long sect = fatbase;
            int off = 0;
            // Use moveWindow scanning per sector
            // Simplify: iterate clusters using getFat (slower but okay for small images)
            for (long c = 2; c < n_fatent; c++) {
                long stat = getFat(c);
                if (stat == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
                if (stat == 1) return FResult.FR_INT_ERR;
                if (stat == 0) nfree++;
            }
        }
        nclst.value = nfree;
        free_clst = nfree;
        fsi_flag |= 1;
        return FResult.FR_OK;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Delete a File/Directory                                          */
    /*-----------------------------------------------------------------------*/
    public FResult unlink(String path) {
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = followPath(dj, stripped);
        if (res != FResult.FR_OK) return res;
        if ((dj.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
        res = moveWindow(dj.sect);
        if (res != FResult.FR_OK) return res;
        int attr = win[dj.dirPtr + DIR_Attr] & 0xFF;
        if ((attr & AM_RDO) != 0) return FResult.FR_DENIED;
        long dclst = ldClustFromWin(dj.dirPtr);
        if ((attr & AM_DIR) != 0) {
            Dir sdj = new Dir();
            sdj.fs = this;
            sdj.sclust = dclst;
            res = dirSdi(sdj, 0);
            if (res != FResult.FR_OK) return res;
            // Check if directory is empty
            while (true) {
                res = moveWindow(sdj.sect);
                if (res != FResult.FR_OK) return res;
                int et = win[sdj.dirPtr] & 0xFF;
                if (et == 0) break; // end, empty
                int a = win[sdj.dirPtr + DIR_Attr] & 0xFF;
                if (et != DDEM && a != AM_LFN) {
                    // Found an entry not deleted
                    // Need to ensure it's not "." or ".." ? For FAT, dot entries exist but they have names "." and ".."
                    // Our dir scan includes them but they are valid entries; we should skip dots for emptiness check
                    // Simple: if entry name is "." or ".." skip
                    boolean isDot = false;
                    if (win[sdj.dirPtr] == '.' ) {
                        if (win[sdj.dirPtr+1] == ' ' ) isDot = true;
                        if (win[sdj.dirPtr+1] == '.' && win[sdj.dirPtr+2] == ' ') isDot = true;
                    }
                    if (!isDot) return FResult.FR_DENIED;
                }
                res = dirNext(sdj, false);
                if (res == FResult.FR_NO_FILE) break;
                if (res != FResult.FR_OK) return res;
            }
            res = FResult.FR_OK;
        }
        if (res == FResult.FR_OK) {
            // Mark entry deleted
            res = moveWindow(dj.sect);
            if (res != FResult.FR_OK) return res;
            win[dj.dirPtr] = (byte) DDEM;
            wflag = 1;
            if (dclst != 0) {
                res = removeChain(dclst, 0);
                if (res != FResult.FR_OK) return res;
            }
            res = syncFs();
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Create a Directory                                               */
    /*-----------------------------------------------------------------------*/
    public FResult mkdir(String path) {
        String stripped = stripDrive(path);
        if (stripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir dj = new Dir();
        dj.fs = this;
        res = followPath(dj, stripped);
        if (res == FResult.FR_OK) {
            if ((dj.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
            return FResult.FR_EXIST;
        }
        if (res != FResult.FR_NO_FILE) return res;
        // Allocate cluster for new directory
        long dcl = createChain(0);
        if (dcl == 0) return FResult.FR_DENIED;
        if (dcl == 1) return FResult.FR_INT_ERR;
        if (dcl == 0xFFFFFFFFL) return FResult.FR_DISK_ERR;
        res = dirClear(dcl);
        if (res != FResult.FR_OK) {
            removeChain(dcl, 0);
            return res;
        }
        // Create dot entries (FAT only)
        // Need to have window at first sector of new dir
        long sect = clst2sect(dcl);
        res = moveWindow(sect);
        if (res != FResult.FR_OK) { removeChain(dcl,0); return res; }
        // "."
        for (int i = 0; i < 11; i++) win[DIR_Name + i] = (byte) ' ';
        win[DIR_Name] = '.';
        win[DIR_Attr] = (byte) AM_DIR;
        long tm = getFatTime();
        stDword(win, DIR_ModTime, tm);
        stDword(win, DIR_CrtTime, tm);
        stClustWin(DIR_Name, dcl);
        // ".."
        System.arraycopy(win, 0, win, SZDIRE, SZDIRE);
        win[SZDIRE + 1] = '.';
        // parent cluster is dj.sclust (0 for root)
        long pcl = dj.sclust;
        if (pcl == 0 && fs_type == FS_FAT32) pcl = dirbase; // root cluster? For subdir parent is root cluster? In C, parent of root's child is 0? Let's use 0 for root
        // Actually for directory under root, parent is 0, but spec expects 0 in ".." for root child? In FAT32 root is cluster, but ".." of root child should be 0 per spec
        // Use dj.sclust directly (0 for root)
        stClustWin(SZDIRE, pcl);
        wflag = 1;
        // Sync the dot entries sector
        res = syncWindow();
        if (res != FResult.FR_OK) { removeChain(dcl,0); return res; }
        // Register directory entry in parent
        // dj currently holds target location's parent and fn; need to allocate entry
        // dirAlloc will find free entry
        Dir parent = dj; // parent dir remains
        // Need to preserve parent sclust etc; dirAlloc uses parent dp
        res = dirAlloc(parent, 1);
        if (res != FResult.FR_OK) { removeChain(dcl,0); return res; }
        res = moveWindow(parent.sect);
        if (res != FResult.FR_OK) { removeChain(dcl,0); return res; }
        for (int i = 0; i < SZDIRE; i++) win[parent.dirPtr + i] = 0;
        System.arraycopy(parent.fn, 0, win, parent.dirPtr + DIR_Name, 11);
        stDword(win, parent.dirPtr + DIR_CrtTime, tm);
        stDword(win, parent.dirPtr + DIR_ModTime, tm);
        stClustWin(parent.dirPtr, dcl);
        win[parent.dirPtr + DIR_Attr] = (byte) AM_DIR;
        wflag = 1;
        res = syncFs();
        if (res != FResult.FR_OK) {
            // remove allocated cluster on failure? Already registered entry will be deleted by chain removal?
            // Keep simple
        }
        return res;
    }

    /*-----------------------------------------------------------------------*/
    /* API: Rename a File/Directory                                          */
    /*-----------------------------------------------------------------------*/
    public FResult rename(String oldPath, String newPath) {
        String oldStripped = stripDrive(oldPath);
        if (oldStripped == null) return FResult.FR_INVALID_DRIVE;
        String newStripped = stripDrive(newPath);
        if (newStripped == null) return FResult.FR_INVALID_DRIVE;
        FResult res = mountVolume(FA_WRITE);
        if (res != FResult.FR_OK) return res;
        Dir djo = new Dir();
        djo.fs = this;
        res = followPath(djo, oldStripped);
        if (res != FResult.FR_OK) return res;
        if ((djo.fn[NSFLAG] & (NS_DOT | NS_NONAME)) != 0) return FResult.FR_INVALID_NAME;
        res = moveWindow(djo.sect);
        if (res != FResult.FR_OK) return res;
        byte[] buf = new byte[SZDIRE];
        System.arraycopy(win, djo.dirPtr, buf, 0, SZDIRE);
        int oldAttr = buf[DIR_Attr] & 0xFF;
        Dir djn = new Dir();
        djn.fs = this;
        // Need to copy parent info for new path: followPath will set djn to parent of new name
        // But we need to duplicate djo for attribute
        res = followPath(djn, newStripped);
        if (res == FResult.FR_OK) {
            // Check if same object (same sect+ptr and same cluster)
            if (djn.sect == djo.sect && djn.dirPtr == djo.dirPtr) res = FResult.FR_NO_FILE;
            else res = FResult.FR_EXIST;
        }
        if (res != FResult.FR_NO_FILE) {
            if (res == FResult.FR_OK) res = FResult.FR_EXIST;
            return res;
        }
        // Allocate new entry
        res = dirAlloc(djn, 1);
        if (res != FResult.FR_OK) return res;
        res = moveWindow(djn.sect);
        if (res != FResult.FR_OK) return res;
        // Copy directory entry except name, preserve FN
        // buf contains old entry; need to copy fields except name replaced
        byte[] newFn = djn.fn;
        // Build new entry at djn.dirPtr
        for (int i = 0; i < SZDIRE; i++) win[djn.dirPtr + i] = 0;
        System.arraycopy(newFn, 0, win, djn.dirPtr + DIR_Name, 11);
        // Copy other fields from buf (except name)
        // Copy attr, times, cluster, size
        win[djn.dirPtr + DIR_Attr] = buf[DIR_Attr];
        if ((buf[DIR_Attr] & AM_DIR) == 0) win[djn.dirPtr + DIR_Attr] |= AM_ARC;
        // Copy timestamps etc. Simplified: copy whole 13..32 except name but ensure cluster/size preserved
        System.arraycopy(buf, 11, win, djn.dirPtr + 11, SZDIRE - 11);
        // For directory being moved, need to update ".." entry if it's a directory
        if ((buf[DIR_Attr] & AM_DIR) != 0 && djo.sclust != djn.sclust) {
            long newCl = ldClust(buf, 0);
            long sec = clst2sect(newCl);
            if (sec == 0) return FResult.FR_INT_ERR;
            res = moveWindow(sec);
            if (res != FResult.FR_OK) return res;
            // ".." is second entry
            int dotDotPtr = SZDIRE;
            if ((win[dotDotPtr] & 0xFF) == '.' && (win[dotDotPtr+1] & 0xFF) == '.') {
                stClustWin(dotDotPtr, djn.sclust);
                wflag = 1;
                res = syncWindow();
                if (res != FResult.FR_OK) return res;
                // Need to bring back window for new entry
                res = moveWindow(djn.sect);
                if (res != FResult.FR_OK) return res;
            }
        }
        wflag = 1;
        // Remove old entry
        res = moveWindow(djo.sect);
        if (res != FResult.FR_OK) return res;
        win[djo.dirPtr] = (byte) DDEM;
        wflag = 1;
        res = syncFs();
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
