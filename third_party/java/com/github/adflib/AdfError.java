/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_err.h
 *
 *  $Id$
 *
 *  error codes
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
 * Return codes translated from {@code adf_err.h}.
 *
 * <p>Mirrors the C {@code RETCODE} defines. Values intentionally overlap across
 * different sub-domains (e.g. {@code RC_MALLOC == RC_BLOCKTYPE == 1}) exactly as
 * in the C headers; callers disambiguate by context. Keep {@link #hasRC} structure
 * as in C: {@code hasRC(rc,c) == (rc & c) != 0}.
 *
 * <p>Use {@code int[1]} / {@code long[1]} arrays for C out-parameters instead of boxed holders.
 */
public enum AdfError {

    /* generic */

    RC_OK(0),
    RC_ERROR(-1),

    RC_MALLOC(1),
    RC_VOLFULL(2),

    RC_FOPEN(1 << 10),
    RC_NULLPTR(1 << 12),

    /* adfRead*Block() */

    RC_BLOCKTYPE(1),
    RC_BLOCKSTYPE(1 << 1),
    RC_BLOCKSUM(1 << 2),
    RC_HEADERKEY(1 << 3),
    RC_BLOCKREAD(1 << 4),

    /* adfWrite*Block — RC_BLOCKWRITE overlaps RC_BLOCKREAD */

    RC_BLOCKWRITE(1 << 4),

    /* adfReadBlock() */

    RC_BLOCKOUTOFRANGE(1),
    RC_BLOCKNATREAD(1 << 1),

    /* adfWriteBlock() — RC_BLOCKOUTOFRANGE reused */
    RC_BLOCKNATWRITE(1 << 1),
    RC_BLOCKREADONLY(1 << 2),

    /* adfNativeReadBlock(), adfReadDumpSector() */

    RC_BLOCKSHORTREAD(1),
    RC_BLOCKFSEEK(1 << 1),

    /* adfNativeWriteBlock(), adfWriteDumpSector() — RC_BLOCKFSEEK reused */

    RC_BLOCKSHORTWRITE(1),

    /* -- adfReadRDSKblock -- */

    RC_BLOCKID(1 << 5);

    /* -- adfWriteRDSKblock() — RC_BLOCKREADONLY reused */

    private final int value;

    AdfError(int value) {
        this.value = value;
    }

    /**
     * Numeric value of this return code, matching the C {@code #define}.
     */
    public int getValue() {
        return value;
    }

    /**
     * Tests whether {@code rc} has flag {@code c} set, mirroring
     * the C macro {@code hasRC(rc,c) ((rc) & (c))}.
     */
    public static boolean hasRC(int rc, int c) {
        return (rc & c) != 0;
    }

    /**
     * Tests whether {@code rc} has the enum flag {@code c} set.
     */
    public static boolean hasRC(int rc, AdfError c) {
        return (rc & c.value) != 0;
    }
}
