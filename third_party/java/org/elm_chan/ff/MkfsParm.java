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

/* Format parameter structure (MKFS_PARM) used for f_mkfs() */

public final class MkfsParm {
    public int fmt; /* Format option (FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD) */
    public int nFat; /* Number of FATs */
    public int align; /* Data area alignment (sector) */
    public int nRoot; /* Number of root directory entries */
    public long auSize; /* Cluster size (byte) */

    public MkfsParm() {}

    public MkfsParm(int fmt, int nFat, int align, int nRoot, long auSize) {
        this.fmt = fmt;
        this.nFat = nFat;
        this.align = align;
        this.nRoot = nRoot;
        this.auSize = auSize;
    }

    /* Default parameter - {FM_ANY, 0, 0, 0, 0} */
    public static MkfsParm defaultParm() {
        return new MkfsParm(FatFs.FM_ANY, 0, 0, 0, 0);
    }
}
