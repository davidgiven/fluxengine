/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bBootBlock
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
 * {@code struct bBootBlock} — 1024 bytes (2 logical sectors).
 *
 * <pre>
 * struct bBootBlock
 * {
 *     char dosType[4];
 *     ULONG checkSum;
 *     int32_t rootBlock;
 *     UCHAR data[500 + 512];
 * };
 * </pre>
 *
 * All multi-byte fields are big-endian on disk.
 */
public final class BBootBlock {

    /*000*/ public byte[] dosType = new byte[4];
    /*004*/ public long checkSum; /* ULONG */
    /*008*/ public int rootBlock;
    /*00c*/ public byte[] data = new byte[500 + 512];

    public BBootBlock() {
    }

    /**
     * Reads a {@code bBootBlock} from {@code buf} at absolute offset {@code off}.
     * Does not change {@code position}/{@code limit}; expects big-endian.
     */
    public static BBootBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BBootBlock blk = new BBootBlock();
        for (int i = 0; i < 4; i++) {
            blk.dosType[i] = b.get(off + i);
        }
        blk.checkSum = b.getInt(off + 4) & 0xFFFFFFFFL;
        blk.rootBlock = b.getInt(off + 8);
        AdfEndian.copyFromBuffer(b, off + 12, blk.data, 0, blk.data.length);
        return blk;
    }

    /**
     * Writes this {@code bBootBlock} to {@code buf} at absolute offset {@code off}.
     * Does not change {@code position}/{@code limit}; writes big-endian.
     */
    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        for (int i = 0; i < 4; i++) {
            b.put(off + i, dosType[i]);
        }
        b.putInt(off + 4, (int) (checkSum & 0xFFFFFFFFL));
        b.putInt(off + 8, rootBlock);
        AdfEndian.copyToBuffer(data, 0, b, off + 12, data.length);
    }

    /** Total on-disk size of this block. */
    public static final int BLOCK_SIZE = 1024;
}
