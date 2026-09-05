/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bLinkBlock
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
 * {@code struct bLinkBlock}.
 *
 * <pre>
 * struct bLinkBlock
 * {
 *     int32_t type;      // == 2
 *     int32_t headerKey; // self pointer
 *     int32_t r1[3];
 *     ULONG checkSum;
 *     char realName[64];
 *     int32_t r2[83];
 *     int32_t days; // last access
 *     int32_t mins;
 *     int32_t ticks;
 *     char nameLen;
 *     char name[MAXNAMELEN + 1];
 *     int32_t r3;
 *     int32_t realEntry;
 *     int32_t nextLink;
 *     int32_t r4[5];
 *     int32_t nextSameHash;
 *     int32_t parent;
 *     int32_t r5;
 *     int32_t secType; // == -4, 4, 3
 * };
 * </pre>
 */
public final class BLinkBlock
{

    /*000*/ public int type;      /* == 2 */
    /*004*/ public int headerKey; /* self pointer */
    public int[] r1 = new int[3];
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public byte[] realName = new byte[64];
    public int[] r2 = new int[83];
    /*1a4*/ public int days; /* last access */
    /*1a8*/ public int mins;
    /*1ac*/ public int ticks;
    /*1b0*/ public byte nameLen;
    /*1b1*/ public byte[] name = new byte[AdfConstants.MAXNAMELEN + 1];
    public int r3;
    /*1d4*/ public int realEntry;
    /*1d8*/ public int nextLink;
    public int[] r4 = new int[5];
    /*1f0*/ public int nextSameHash;
    /*1f4*/ public int parent;
    public int r5;
    /*1fc*/ public int secType; /* == -4, 4, 3 */

    public BLinkBlock()
    {
    }

    public static BLinkBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BLinkBlock blk = new BLinkBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        for (int i = 0; i < 3; i++)
        {
            blk.r1[i] = b.getInt(off + 8 + i * 4);
        }
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        for (int i = 0; i < 64; i++)
        {
            blk.realName[i] = b.get(off + 24 + i);
        }
        for (int i = 0; i < 83; i++)
        {
            blk.r2[i] = b.getInt(off + 88 + i * 4);
        }
        blk.days = b.getInt(off + 0x1a4);
        blk.mins = b.getInt(off + 0x1a8);
        blk.ticks = b.getInt(off + 0x1ac);
        blk.nameLen = b.get(off + 0x1b0);
        for (int i = 0; i < blk.name.length; i++)
        {
            blk.name[i] = b.get(off + 0x1b1 + i);
        }
        blk.r3 = b.getInt(off + 0x1d0);
        blk.realEntry = b.getInt(off + 0x1d4);
        blk.nextLink = b.getInt(off + 0x1d8);
        for (int i = 0; i < 5; i++)
        {
            blk.r4[i] = b.getInt(off + 0x1dc + i * 4);
        }
        blk.nextSameHash = b.getInt(off + 0x1f0);
        blk.parent = b.getInt(off + 0x1f4);
        blk.r5 = b.getInt(off + 0x1f8);
        blk.secType = b.getInt(off + 0x1fc);
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        for (int i = 0; i < 3; i++)
        {
            b.putInt(off + 8 + i * 4, r1[i]);
        }
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        for (int i = 0; i < 64; i++)
        {
            b.put(off + 24 + i, realName[i]);
        }
        for (int i = 0; i < 83; i++)
        {
            b.putInt(off + 88 + i * 4, r2[i]);
        }
        b.putInt(off + 0x1a4, days);
        b.putInt(off + 0x1a8, mins);
        b.putInt(off + 0x1ac, ticks);
        b.put(off + 0x1b0, nameLen);
        for (int i = 0; i < name.length; i++)
        {
            b.put(off + 0x1b1 + i, name[i]);
        }
        b.putInt(off + 0x1d0, r3);
        b.putInt(off + 0x1d4, realEntry);
        b.putInt(off + 0x1d8, nextLink);
        for (int i = 0; i < 5; i++)
        {
            b.putInt(off + 0x1dc + i * 4, r4[i]);
        }
        b.putInt(off + 0x1f0, nextSameHash);
        b.putInt(off + 0x1f4, parent);
        b.putInt(off + 0x1f8, r5);
        b.putInt(off + 0x1fc, secType);
    }

    public static final int BLOCK_SIZE = 512;
}
