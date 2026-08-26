/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bOFSDataBlock
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
 * {@code struct bOFSDataBlock}.
 *
 * <pre>
 * struct bOFSDataBlock
 * {
 *     int32_t type;      // == 8
 *     int32_t headerKey; // pointer to file_hdr block
 *     int32_t seqNum;    // file data block number
 *     int32_t dataSize;  // <= 0x1e8
 *     int32_t nextData;  // next data block
 *     ULONG checkSum;
 *     UCHAR data[488];
 * };
 * </pre>
 */
public final class BOFSDataBlock {

    /*000*/ public int type;      /* == 8 */
    /*004*/ public int headerKey; /* pointer to file_hdr block */
    /*008*/ public int seqNum;    /* file data block number */
    /*00c*/ public int dataSize;  /* <= 0x1e8 */
    /*010*/ public int nextData;  /* next data block */
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public byte[] data = new byte[488];

    public BOFSDataBlock() {
    }

    public static BOFSDataBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BOFSDataBlock blk = new BOFSDataBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        blk.seqNum = b.getInt(off + 8);
        blk.dataSize = b.getInt(off + 12);
        blk.nextData = b.getInt(off + 16);
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        AdfEndian.copyFromBuffer(b, off + 24, blk.data, 0, blk.data.length);
        return blk;
    }

    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        b.putInt(off + 8, seqNum);
        b.putInt(off + 12, dataSize);
        b.putInt(off + 16, nextData);
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        AdfEndian.copyToBuffer(data, 0, b, off + 24, data.length);
    }

    public static final int BLOCK_SIZE = 512;
}
