/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  hd_blk.h — bLSEGblock
 *
 *  $Id$
 *
 *  hard disk blocks structures
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
 * {@code struct bLSEGblock}.
 *
 * <pre>
 * struct bLSEGblock
 * {
 *     char id[4];   // LSEG
 *     int32_t size; // 128 int32_ts
 *     ULONG checksum;
 *     int32_t hostID; // 7
 *     int32_t next;
 *     char loadData[123 * 4];
 * };
 * </pre>
 */
public final class BLSEGBlock {

    /*000*/ public byte[] id = new byte[4];   /* LSEG */
    /*004*/ public int size; /* 128 int32_ts */
    /*008*/ public long checksum; /* ULONG */
    /*00c*/ public int hostID; /* 7 */
    /*010*/ public int next;
    /*014*/ public byte[] loadData = new byte[123 * 4];

    public BLSEGBlock() {
    }

    public static BLSEGBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BLSEGBlock blk = new BLSEGBlock();
        for (int i = 0; i < 4; i++) blk.id[i] = b.get(off + i);
        blk.size = b.getInt(off + 4);
        blk.checksum = b.getInt(off + 8) & 0xFFFFFFFFL;
        blk.hostID = b.getInt(off + 12);
        blk.next = b.getInt(off + 16);
        AdfEndian.copyFromBuffer(b, off + 20, blk.loadData, 0, blk.loadData.length);
        return blk;
    }

    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        for (int i = 0; i < 4; i++) b.put(off + i, id[i]);
        b.putInt(off + 4, size);
        b.putInt(off + 8, (int) (checksum & 0xFFFFFFFFL));
        b.putInt(off + 12, hostID);
        b.putInt(off + 16, next);
        AdfEndian.copyToBuffer(loadData, 0, b, off + 20, loadData.length);
    }

    public static final int BLOCK_SIZE = 512;
}
