/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_defs.h + adf_blk.h + hd_blk.h + adf_str.h — constants
 *
 *  $Id$
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

package com.github.adflib;

/**
 * Constants translated from {@code adf_defs.h}, {@code adf_blk.h}, {@code hd_blk.h},
 * and {@code adf_str.h}. Mirrors the C {@code #define} values verbatim.
 *
 * <p>Macro helpers ({@code isFFS}, {@code hasR}, etc.) are provided as static
 * methods that keep the original C expression structure.
 */
public final class AdfConstants {

    private AdfConstants() {
    }

    /* adf_defs.h */

    public static final String ADFLIB_VERSION = "0.7.11a";
    public static final String ADFLIB_DATE = "January 20th, 2007";

    public static final int TRUE = 1;
    public static final int FALSE = 0;

    /* (*byte) to (*short) and (*byte) to (*long) conversion — see AdfEndian.Short/Long */

    /* swap short and swap long macros for little endian machines — see AdfEndian.swapShort/swapLong */

    /* adf_blk.h */

    public static final int LOGICAL_BLOCK_SIZE = 512;

    /* ----- FILE SYSTEM ----- */

    public static final int FSMASK_FFS = 1;
    public static final int FSMASK_INTL = 2;
    public static final int FSMASK_DIRCACHE = 4;

    public static final int FS_FFS = FSMASK_FFS;
    public static final int FS_OFS = 0;
    public static final int FS_INTL = FSMASK_INTL;
    public static final int FS_DIRCACHE = FSMASK_DIRCACHE;

    public static boolean isFFS(int c) {
        return (c & FSMASK_FFS) != 0;
    }

    public static boolean isOFS(int c) {
        return (c & FSMASK_FFS) == 0;
    }

    public static boolean isINTL(int c) {
        return (c & FSMASK_INTL) != 0;
    }

    public static boolean isDIRCACHE(int c) {
        return (c & FSMASK_DIRCACHE) != 0;
    }

    /* ----- ENTRIES ----- */

    /* access constants */

    public static final int ACCMASK_D = (1 << 0);
    public static final int ACCMASK_E = (1 << 1);
    public static final int ACCMASK_W = (1 << 2);
    public static final int ACCMASK_R = (1 << 3);
    public static final int ACCMASK_A = (1 << 4);
    public static final int ACCMASK_P = (1 << 5);
    public static final int ACCMASK_S = (1 << 6);
    public static final int ACCMASK_H = (1 << 7);

    public static boolean hasD(int c) {
        return (c & ACCMASK_D) != 0;
    }

    public static boolean hasE(int c) {
        return (c & ACCMASK_E) != 0;
    }

    public static boolean hasW(int c) {
        return (c & ACCMASK_W) != 0;
    }

    public static boolean hasR(int c) {
        return (c & ACCMASK_R) != 0;
    }

    public static boolean hasA(int c) {
        return (c & ACCMASK_A) != 0;
    }

    public static boolean hasP(int c) {
        return (c & ACCMASK_P) != 0;
    }

    public static boolean hasS(int c) {
        return (c & ACCMASK_S) != 0;
    }

    public static boolean hasH(int c) {
        return (c & ACCMASK_H) != 0;
    }

    /* ----- BLOCKS ----- */

    /* block constants */

    public static final int BM_VALID = -1;
    public static final int BM_INVALID = 0;

    public static final int HT_SIZE = 72;
    public static final int BM_SIZE = 25;
    public static final int MAX_DATABLK = 72;

    public static final int MAXNAMELEN = 30;
    public static final int MAXCMMTLEN = 79;

    /* block primary and secondary types */

    public static final int T_HEADER = 2;
    public static final int ST_ROOT = 1;
    public static final int ST_DIR = 2;
    public static final int ST_FILE = -3;
    public static final int ST_LFILE = -4;
    public static final int ST_LDIR = 4;
    public static final int ST_LSOFT = 3;
    public static final int T_LIST = 16;
    public static final int T_DATA = 8;
    public static final int T_DIRC = 33;

    /* adf_str.h — device types */

    public static final int DEVTYPE_FLOPDD = 1;
    public static final int DEVTYPE_FLOPHD = 2;
    public static final int DEVTYPE_HARDDISK = 3;
    public static final int DEVTYPE_HARDFILE = 4;

    /* adf_str.h — environment callback selectors */

    public static final int PR_VFCT = 1;
    public static final int PR_WFCT = 2;
    public static final int PR_EFCT = 3;
    public static final int PR_NOTFCT = 4;
    public static final int PR_USEDIRC = 5;
    public static final int PR_USE_NOTFCT = 6;
    public static final int PR_PROGBAR = 7;
    public static final int PR_USE_PROGBAR = 8;
    public static final int PR_RWACCESS = 9;
    public static final int PR_USE_RWACCESS = 10;

    /* defines max and min — Java helpers mirroring the C macros */

    public static int max(int a, int b) {
        return a > b ? a : b;
    }

    public static int min(int a, int b) {
        return a < b ? a : b;
    }
}
