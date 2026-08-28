/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * Java port of constants from hfs.h, libhfs.h, low.h, and apple.h.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

package org.mars.hfsutils;

/**
 * All constants from the C headers ({@code hfs.h}, {@code libhfs.h},
 * {@code low.h}, {@code apple.h}).
 */
public final class HfsConstants
{
    private HfsConstants()
    {
    }

    /* -----------------------------------------------------------------------
     * hfs.h
     * ----------------------------------------------------------------------- */

    public static final int HFS_BLOCKSZ      = 512;
    public static final int HFS_BLOCKSZ_BITS = 9;

    public static final int HFS_MAX_FLEN = 31;
    public static final int HFS_MAX_VLEN = 27;

    /* hfsdirent flags */
    public static final int HFS_ISDIR    = 0x0001;
    public static final int HFS_ISLOCKED = 0x0002;

    /* CNID values */
    public static final int HFS_CNID_ROOTPAR = 1;
    public static final int HFS_CNID_ROOTDIR = 2;
    public static final int HFS_CNID_EXT     = 3;
    public static final int HFS_CNID_CAT     = 4;
    public static final int HFS_CNID_BADALLOC = 5;

    /* Finder flags */
    public static final int HFS_FNDR_ISONDESK               = 1;
    public static final int HFS_FNDR_COLOR                   = 0x0e;
    public static final int HFS_FNDR_COLORRESERVED           = 1 << 4;
    public static final int HFS_FNDR_REQUIRESSWITCHLAUNCH    = 1 << 5;
    public static final int HFS_FNDR_ISSHARED                = 1 << 6;
    public static final int HFS_FNDR_HASNOINITS              = 1 << 7;
    public static final int HFS_FNDR_HASBEENINITED           = 1 << 8;
    public static final int HFS_FNDR_RESERVED                = 1 << 9;
    public static final int HFS_FNDR_HASCUSTOMICON           = 1 << 10;
    public static final int HFS_FNDR_ISSTATIONERY            = 1 << 11;
    public static final int HFS_FNDR_NAMELOCKED              = 1 << 12;
    public static final int HFS_FNDR_HASBUNDLE               = 1 << 13;
    public static final int HFS_FNDR_ISINVISIBLE             = 1 << 14;
    public static final int HFS_FNDR_ISALIAS                 = 1 << 15;

    /* mount modes */
    public static final int HFS_MODE_RDONLY = 0;
    public static final int HFS_MODE_RDWR   = 1;
    public static final int HFS_MODE_ANY    = 2;

    public static final int HFS_MODE_MASK = 0x0003;

    /* mount options */
    public static final int HFS_OPT_NOCACHE = 0x0100;
    public static final int HFS_OPT_2048    = 0x0200;
    public static final int HFS_OPT_ZERO    = 0x0400;

    /* seek constants */
    public static final int HFS_SEEK_SET = 0;
    public static final int HFS_SEEK_CUR = 1;
    public static final int HFS_SEEK_END = 2;

    /* -----------------------------------------------------------------------
     * libhfs.h — bucket, cache, node, record, and file sizes
     * ----------------------------------------------------------------------- */

    public static final int HFS_CACHESZ    = 128;
    public static final int HFS_HASHSZ     = 32;
    public static final int HFS_BLOCKBUFSZ = 16;

    public static final int HFS_BUCKET_INUSE = 0x01;
    public static final int HFS_BUCKET_DIRTY = 0x02;

    public static final int HFS_MAP1SZ  = 256;
    public static final int HFS_MAPXSZ  = 492;

    public static final int HFS_MAX_NRECS = 35;

    /* Record key/data length helpers (operate on byte[] offsets) */
    public static final int HFS_MAX_KEYLEN  = 41; /* sizeof(CatKeyRec) vs ExtKeyRec; max used */
    public static final int HFS_MAX_DATALEN = 260;
    public static final int HFS_MAX_RECLEN  = HFS_MAX_KEYLEN + HFS_MAX_DATALEN;

    public static final int HFS_CATKEYLEN = 38; /* sizeof(CatKeyRec) */
    public static final int HFS_EXTKEYLEN = 16; /* sizeof(ExtKeyRec) */
    public static final int HFS_MAX_CATRECLEN = HFS_CATKEYLEN + HFS_MAX_DATALEN;
    public static final int HFS_MAX_EXTRECLEN = HFS_EXTKEYLEN + 12;

    /* Signature words */
    public static final int HFS_SIGWORD     = 0x4244;
    public static final short HFS_SIGWORD_MFS = (short) 0xd2d7;

    /* MDB attribute bits */
    public static final int HFS_ATRB_BUSY     = 1 << 6;
    public static final int HFS_ATRB_HLOCKED  = 1 << 7;
    public static final int HFS_ATRB_UMOUNTED = 1 << 8;
    public static final int HFS_ATRB_BBSPARED = 1 << 9;
    public static final int HFS_ATRB_BVINCONSIS = 1 << 11;
    public static final int HFS_ATRB_COPYPROT = 1 << 14;
    public static final int HFS_ATRB_SLOCKED  = 1 << 15;

    /* volume flags */
    public static final int HFS_VOL_OPEN          = 0x0001;
    public static final int HFS_VOL_MOUNTED       = 0x0002;
    public static final int HFS_VOL_READONLY      = 0x0004;
    public static final int HFS_VOL_USINGCACHE    = 0x0008;

    public static final int HFS_VOL_UPDATE_MDB    = 0x0010;
    public static final int HFS_VOL_UPDATE_ALTMDB = 0x0020;
    public static final int HFS_VOL_UPDATE_VBM    = 0x0040;

    public static final int HFS_VOL_OPT_MASK = 0xff00;

    /* file flags */
    public static final int HFS_FILE_UPDATE_CATREC = 0x01;

    /* B*-tree flags */
    public static final int HFS_BT_UPDATE_HDR = 0x01;

    /* B*-tree node types (from apple.h) */
    public static final byte ndIndxNode = (byte) 0x00;
    public static final byte ndHdrNode  = (byte) 0x01;
    public static final byte ndMapNode  = (byte) 0x02;
    public static final byte ndLeafNode = (byte) 0xff;

    /* -----------------------------------------------------------------------
     * low.h
     * ----------------------------------------------------------------------- */

    public static final int HFS_DDR_SIGWORD   = 0x4552;
    public static final int HFS_PM_SIGWORD    = 0x504d;
    public static final int HFS_PM_SIGWORD_OLD = 0x5453;
    public static final int HFS_BB_SIGWORD    = 0x4c4b;

    public static final int HFS_BOOTCODE1LEN = HFS_BLOCKSZ - 148;
    public static final int HFS_BOOTCODE2LEN = HFS_BLOCKSZ;
    public static final int HFS_BOOTCODELEN  = HFS_BOOTCODE1LEN + HFS_BOOTCODE2LEN;

    /* -----------------------------------------------------------------------
     * file.h — fork type enum
     * ----------------------------------------------------------------------- */

    public static final int FK_DATA = 0x00;
    public static final int FK_RSRC = (byte) 0xff;
}
