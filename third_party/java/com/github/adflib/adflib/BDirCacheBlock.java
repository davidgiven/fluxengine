/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bDirCacheBlock
 *
 *  $Id$
 *
 *  general blocks structures
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

package com.github.adflib.adflib;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * {@code struct bDirCacheBlock} — directory cache block.
 *
 * <pre>
 * struct bDirCacheBlock
 * {
 *     int32_t type; // == 33
 *     int32_t headerKey;
 *     int32_t parent;
 *     int32_t recordsNb;
 *     int32_t nextDirC;
 *     ULONG checkSum;
 *     uint8_t records[488];
 * };
 * </pre>
 */
public final class BDirCacheBlock
{

    /*000*/ public int type; /* == 33 */
    /*004*/ public int headerKey;
    /*008*/ public int parent;
    /*00c*/ public int recordsNb;
    /*010*/ public int nextDirC;
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public byte[] records = new byte[488];

    public BDirCacheBlock()
    {
    }

    public static BDirCacheBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BDirCacheBlock blk = new BDirCacheBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        blk.parent = b.getInt(off + 8);
        blk.recordsNb = b.getInt(off + 12);
        blk.nextDirC = b.getInt(off + 16);
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        AdfEndian.copyFromBuffer(b, off + 24, blk.records, 0, blk.records.length);
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        b.putInt(off + 8, parent);
        b.putInt(off + 12, recordsNb);
        b.putInt(off + 16, nextDirC);
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        AdfEndian.copyToBuffer(records, 0, b, off + 24, records.length);
    }

    public static final int BLOCK_SIZE = 512;
}
