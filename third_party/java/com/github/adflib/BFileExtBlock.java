/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bFileExtBlock
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

package com.github.adflib;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * {@code struct bFileExtBlock} — file header extension block.
 *
 * <pre>
 * struct bFileExtBlock
 * {
 *     int32_t type; // == 0x10
 *     int32_t headerKey;
 *     int32_t highSeq;
 *     int32_t dataSize;  // == 0
 *     int32_t firstData; // == 0
 *     ULONG checkSum;
 *     int32_t dataBlocks[MAX_DATABLK];
 *     int32_t r[45];
 *     int32_t info;              // == 0
 *     int32_t nextSameHash;      // == 0
 *     int32_t parent;    // header block
 *     int32_t extension; // next header extension block
 *     int32_t secType;   // -3
 * };
 * </pre>
 */
public final class BFileExtBlock {

    /*000*/ public int type; /* == 0x10 */
    /*004*/ public int headerKey;
    /*008*/ public int highSeq;
    /*00c*/ public int dataSize;  /* == 0 */
    /*010*/ public int firstData; /* == 0 */
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public int[] dataBlocks = new int[AdfConstants.MAX_DATABLK];
    public int[] r = new int[45];
    public int info;              /* == 0 */
    public int nextSameHash;      /* == 0 */
    /*1f4*/ public int parent;    /* header block */
    /*1f8*/ public int extension; /* next header extension block */
    /*1fc*/ public int secType;   /* -3 */

    public BFileExtBlock() {
    }

    public static BFileExtBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BFileExtBlock blk = new BFileExtBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        blk.highSeq = b.getInt(off + 8);
        blk.dataSize = b.getInt(off + 12);
        blk.firstData = b.getInt(off + 16);
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        for (int i = 0; i < AdfConstants.MAX_DATABLK; i++) {
            blk.dataBlocks[i] = b.getInt(off + 24 + i * 4);
        }
        for (int i = 0; i < 45; i++) {
            blk.r[i] = b.getInt(off + 24 + AdfConstants.MAX_DATABLK * 4 + i * 4);
        }
        blk.info = b.getInt(off + 0x1ec);
        blk.nextSameHash = b.getInt(off + 0x1f0);
        blk.parent = b.getInt(off + 0x1f4);
        blk.extension = b.getInt(off + 0x1f8);
        blk.secType = b.getInt(off + 0x1fc);
        return blk;
    }

    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        b.putInt(off + 8, highSeq);
        b.putInt(off + 12, dataSize);
        b.putInt(off + 16, firstData);
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        for (int i = 0; i < AdfConstants.MAX_DATABLK; i++) {
            b.putInt(off + 24 + i * 4, dataBlocks[i]);
        }
        for (int i = 0; i < 45; i++) {
            b.putInt(off + 24 + AdfConstants.MAX_DATABLK * 4 + i * 4, r[i]);
        }
        b.putInt(off + 0x1ec, info);
        b.putInt(off + 0x1f0, nextSameHash);
        b.putInt(off + 0x1f4, parent);
        b.putInt(off + 0x1f8, extension);
        b.putInt(off + 0x1fc, secType);
    }

    public static final int BLOCK_SIZE = 512;
}
