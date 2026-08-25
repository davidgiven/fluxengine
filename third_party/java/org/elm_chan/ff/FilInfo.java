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

/* File/directory information structure (FILINFO) */

public final class FilInfo {
    public long fsize; /* File size (invalid for directory) */
    public int fdate; /* Date of file modification or directory creation */
    public int ftime; /* Time of file modification or directory creation */
    /* WORD crdate; */ /* Date of object createion - pruned: FF_FS_CRTIME == 0 */
    /* WORD crtime; */ /* Time of object createion - pruned: FF_FS_CRTIME == 0 */
    public int fattrib; /* Object attribute */
    /* TCHAR altname[FF_SFN_BUF + 1]; */ /* Alternative object name - pruned: FF_USE_LFN == 0 */
    /* TCHAR fname[FF_LFN_BUF + 1]; */ /* Primary object name - pruned: FF_USE_LFN == 0, replaced by SFN */
    public String fname = ""; /* Object name */

    /* File attribute bits for directory entry (FILINFO.fattrib) */
    public static final int AM_RDO = 0x01 /* Read only */;
    public static final int AM_HID = 0x02 /* Hidden */;
    public static final int AM_SYS = 0x04 /* System */;
    public static final int AM_VOL = 0x08 /* Volume label */;
    public static final int AM_LFN = 0x0F /* LFN entry */;
    public static final int AM_DIR = 0x10 /* Directory */;
    public static final int AM_ARC = 0x20 /* Archive */;
    public static final int AM_MASK = 0x3F /* Mask of defined bits in FAT */;
    /* AM_MASKX 0x37 - pruned: Mask of defined bits in exFAT - exFAT disabled */
}
