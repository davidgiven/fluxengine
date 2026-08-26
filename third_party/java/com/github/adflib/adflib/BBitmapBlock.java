/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bBitmapBlock
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
 * {@code struct bBitmapBlock}.
 *
 * <pre>
 * struct bBitmapBlock
 * {
 *     ULONG checkSum;
 *     ULONG map[127];
 * };
 * </pre>
 */
public final class BBitmapBlock
{

    /*000*/ public long checkSum; /* ULONG */
    /*004*/ public long[] map = new long[127]; /* ULONG[127] */

    public BBitmapBlock()
    {
    }

    public static BBitmapBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BBitmapBlock blk = new BBitmapBlock();
        blk.checkSum = b.getInt(off + 0) & 0xFFFFFFFFL;
        for (int i = 0; i < 127; i++)
        {
            blk.map[i] = b.getInt(off + 4 + i * 4) & 0xFFFFFFFFL;
        }
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, (int) (checkSum & 0xFFFFFFFFL));
        for (int i = 0; i < 127; i++)
        {
            b.putInt(off + 4 + i * 4, (int) (map[i] & 0xFFFFFFFFL));
        }
    }

    public static final int BLOCK_SIZE = 512;
}
