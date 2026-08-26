/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bBitmapExtBlock
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
 * {@code struct bBitmapExtBlock}.
 *
 * <pre>
 * struct bBitmapExtBlock
 * {
 *     int32_t bmPages[127];
 *     int32_t nextBlock;
 * };
 * </pre>
 */
public final class BBitmapExtBlock
{

    /*000*/ public int[] bmPages = new int[127];
    /*1fc*/ public int nextBlock;

    public BBitmapExtBlock()
    {
    }

    public static BBitmapExtBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BBitmapExtBlock blk = new BBitmapExtBlock();
        for (int i = 0; i < 127; i++)
        {
            blk.bmPages[i] = b.getInt(off + i * 4);
        }
        blk.nextBlock = b.getInt(off + 0x1fc);
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        for (int i = 0; i < 127; i++)
        {
            b.putInt(off + i * 4, bmPages[i]);
        }
        b.putInt(off + 0x1fc, nextBlock);
    }

    public static final int BLOCK_SIZE = 512;
}
