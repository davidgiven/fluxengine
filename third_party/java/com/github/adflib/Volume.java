/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct Volume
 *
 *  $Id$
 *
 *  structures/constants definitions
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
 * {@code struct Volume}.
 *
 * <pre>
 * struct Volume
 * {
 *     struct Device* dev;
 *     SECTNUM firstBlock;
 *     SECTNUM lastBlock;
 *     SECTNUM rootBlock;
 *     char dosType;
 *     BOOL bootCode;
 *     BOOL readOnly;
 *     int datablockSize;
 *     int blockSize;
 *     char* volName;
 *     BOOL mounted;
 *     int32_t bitmapSize;
 *     SECTNUM* bitmapBlocks;
 *     struct bBitmapBlock** bitmapTable;
 *     BOOL* bitmapBlocksChg;
 *     SECTNUM curDirPtr;
 * };
 * </pre>
 *
 * High-level object — plain POJO, not a ByteBuffer overlay.
 * {@code SECTNUM} maps to {@code int}; {@code BOOL} to {@code boolean}.
 */
public final class Volume {

    public Device dev;

    public int firstBlock;    /* first block of data area (from beginning of device) */
    public int lastBlock; /* last block of data area  (from beginning of device) */
    public int rootBlock; /* root block (from firstBlock) */

    public byte dosType; /* FFS/OFS, DIRCACHE, INTERNATIONAL */
    public boolean bootCode;
    public boolean readOnly;
    public int datablockSize; /* 488 or 512 */
    public int blockSize;     /* 512 */

    public String volName;

    public boolean mounted;

    public int bitmapSize;    /* in blocks */
    public int[] bitmapBlocks; /* bitmap blocks pointers */
    public BBitmapBlock[] bitmapTable;
    public boolean[] bitmapBlocksChg;

    public int curDirPtr;

    public Volume() {
    }
}
