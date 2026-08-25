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

/* File object structure (FIL) */
/* Object ID and allocation information (FFOBJID) is embedded as fields fs/id/attr/sclust/objsize */

public final class Fil {
    /* FFOBJID obj; */ /* Object identifier (must be the 1st member to detect invalid object pointer) */
    public FatFs fs; /* Pointer to the volume holding this object */
    public int id; /* Volume mount ID when this object was opened */
    public int attr; /* Object attribute */
    /* BYTE stat; */ /* Object chain status (exFAT: b1-0: =0:not contiguous, =2:contiguous, =3:fragmented in this session, b2:sub-directory stretched) - pruned: FF_FS_EXFAT == 0, field omitted */
    public long sclust; /* Object data cluster (0:no data or root directory) */
    public long objsize; /* Object size (valid when sclust != 0) */
    /* DWORD n_cont; */ /* Size of first fragment - 1 (valid when stat == 3) - pruned: exFAT disabled */
    /* DWORD n_frag; */ /* Size of last fragment needs to be written to FAT (valid when not zero) - pruned: exFAT disabled */
    /* DWORD c_scl; */ /* Cluster of directory holding this object (valid when sclust != 0) - pruned: exFAT disabled */
    /* DWORD c_size; */ /* Size of directory holding this object (b7-b0: allocation status, valid when c_scl != 0) - pruned: exFAT disabled */
    /* DWORD c_ofs; */ /* Offset of entry in the holding directory - pruned: exFAT disabled */
    /* UINT lockid; */ /* File lock ID origin from 1 (index of file semaphore table Files[]) - pruned: FF_FS_LOCK == 0, field omitted */

    public int flag; /* File status flags */
    public int err; /* Abort flag (error code) */
    public long fptr; /* File read/write pointer (0 on open) */
    public long clust; /* Current cluster of fptr (invalid when fptr is 0) */
    public long sect; /* Sector number appearing in buf[] (0:invalid) */
    public long dirSect; /* Sector number containing the directory entry (not used in exFAT) */
    public int dirPtr; /* Pointer to the directory entry in the win[] (not used in exFAT) */
    /* DWORD* cltbl; */ /* Pointer to the cluster link map table (nulled on open; set by application) - pruned: FF_USE_FASTSEEK == 0 */
    public byte[] buf = new byte[FatFs.FF_MAX_SS]; /* File private data read/write window */
}
