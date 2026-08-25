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

/* Directory object structure (DIR) */

public final class Dir {
    /* FFOBJID obj; */ /* Object identifier (must be the 1st member to detect invalid object pointer) */
    public FatFs fs; /* Pointer to the volume holding this object */
    public int id; /* Volume mount ID when this object was opened */
    public int attr; /* Object attribute */
    /* BYTE stat; */ /* Object chain status (exFAT: b1-0: =0:not contiguous, =2:contiguous, =3:fragmented in this session, b2:sub-directory stretched) - pruned: FF_FS_EXFAT == 0 */
    public long sclust; /* Object data cluster (0:no data or root directory) */
    public long objsize; /* Object size (valid when sclust != 0) - for DIR this is directory size tracking; kept for parity with FFOBJID */
    /* DWORD n_cont; DWORD n_frag; DWORD c_scl; DWORD c_size; DWORD c_ofs; - pruned: exFAT disabled */
    /* UINT lockid; - pruned: FF_FS_LOCK == 0 */

    public long dptr; /* Current read/write offset */
    public long clust; /* Current cluster */
    public long sect; /* Current sector (0:no more item to read) */
    public int dirPtr; /* Pointer to the directory item in the win[] in filesystem object */
    public byte[] fn = new byte[12]; /* SFN (in/out) {body[0-7],ext[8-10],status[11]} */
    /* DWORD blk_ofs; */ /* Offset of current entry block being processed (0xFFFFFFFF:invalid) - pruned: FF_USE_LFN == 0, field omitted */
    /* const TCHAR *pat; */ /* Pointer to the name matching pattern - pruned: FF_USE_FIND == 0, field omitted */
}
