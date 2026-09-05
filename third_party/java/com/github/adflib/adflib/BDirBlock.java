/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bDirBlock
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
 * {@code struct bDirBlock}.
 *
 * <pre>
 * struct bDirBlock
 * {
 *     int32_t type; // == 2
 *     int32_t headerKey;
 *     int32_t highSeq;       // == 0
 *     int32_t hashTableSize; // == 0
 *     int32_t r1;                    // == 0
 *     ULONG checkSum;
 *     int32_t hashTable[HT_SIZE]; // hash table
 *     int32_t r2[2];
 *     int32_t access;
 *     int32_t r4; // == 0
 *     char commLen;
 *     char comment[MAXCMMTLEN + 1];
 *     char r5[91 - (MAXCMMTLEN + 1)];
 *     int32_t days; // last access
 *     int32_t mins;
 *     int32_t ticks;
 *     char nameLen;
 *     char dirName[MAXNAMELEN + 1];
 *     int32_t r6;
 *     int32_t real;     // ==0
 *     int32_t nextLink; // link list
 *     int32_t r7[5];
 *     int32_t nextSameHash;
 *     int32_t parent;
 *     int32_t extension; // FFS : first directory cache
 *     int32_t secType;   // == 2
 * };
 * </pre>
 */
public final class BDirBlock
{

    /*000*/ public int type; /* == 2 */
    /*004*/ public int headerKey;
    /*008*/ public int highSeq;       /* == 0 */
    /*00c*/ public int hashTableSize; /* == 0 */
    public int r1;                    /* == 0 */
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public int[] hashTable = new int[AdfConstants.HT_SIZE]; /* hash table */
    public int[] r2 = new int[2];
    /*140*/ public int access;
    public int r4; /* == 0 */
    /*148*/ public byte commLen;
    /*149*/ public byte[] comment = new byte[AdfConstants.MAXCMMTLEN + 1];
    public byte[] r5 = new byte[91 - (AdfConstants.MAXCMMTLEN + 1)];
    /*1a4*/ public int days; /* last access */
    /*1a8*/ public int mins;
    /*1ac*/ public int ticks;
    /*1b0*/ public byte nameLen;
    /*1b1*/ public byte[] dirName = new byte[AdfConstants.MAXNAMELEN + 1];
    public int r6;
    /*1d4*/ public int real;     /* ==0 */
    /*1d8*/ public int nextLink; /* link list */
    public int[] r7 = new int[5];
    /*1f0*/ public int nextSameHash;
    /*1f4*/ public int parent;
    /*1f8*/ public int extension; /* FFS : first directory cache */
    /*1fc*/ public int secType;   /* == 2 */

    public BDirBlock()
    {
    }

    public static BDirBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BDirBlock blk = new BDirBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        blk.highSeq = b.getInt(off + 8);
        blk.hashTableSize = b.getInt(off + 12);
        blk.r1 = b.getInt(off + 16);
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        for (int i = 0; i < AdfConstants.HT_SIZE; i++)
        {
            blk.hashTable[i] = b.getInt(off + 24 + i * 4);
        }
        blk.r2[0] = b.getInt(off + 0x138);
        blk.r2[1] = b.getInt(off + 0x13c);
        blk.access = b.getInt(off + 0x140);
        blk.r4 = b.getInt(off + 0x144);
        blk.commLen = b.get(off + 0x148);
        for (int i = 0; i < blk.comment.length; i++)
        {
            blk.comment[i] = b.get(off + 0x149 + i);
        }
        for (int i = 0; i < blk.r5.length; i++)
        {
            blk.r5[i] = b.get(off + 0x149 + blk.comment.length + i);
        }
        blk.days = b.getInt(off + 0x1a4);
        blk.mins = b.getInt(off + 0x1a8);
        blk.ticks = b.getInt(off + 0x1ac);
        blk.nameLen = b.get(off + 0x1b0);
        for (int i = 0; i < blk.dirName.length; i++)
        {
            blk.dirName[i] = b.get(off + 0x1b1 + i);
        }
        blk.r6 = b.getInt(off + 0x1d0);
        blk.real = b.getInt(off + 0x1d4);
        blk.nextLink = b.getInt(off + 0x1d8);
        for (int i = 0; i < 5; i++)
        {
            blk.r7[i] = b.getInt(off + 0x1dc + i * 4);
        }
        blk.nextSameHash = b.getInt(off + 0x1f0);
        blk.parent = b.getInt(off + 0x1f4);
        blk.extension = b.getInt(off + 0x1f8);
        blk.secType = b.getInt(off + 0x1fc);
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        b.putInt(off + 8, highSeq);
        b.putInt(off + 12, hashTableSize);
        b.putInt(off + 16, r1);
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        for (int i = 0; i < AdfConstants.HT_SIZE; i++)
        {
            b.putInt(off + 24 + i * 4, hashTable[i]);
        }
        b.putInt(off + 0x138, r2[0]);
        b.putInt(off + 0x13c, r2[1]);
        b.putInt(off + 0x140, access);
        b.putInt(off + 0x144, r4);
        b.put(off + 0x148, commLen);
        for (int i = 0; i < comment.length; i++)
        {
            b.put(off + 0x149 + i, comment[i]);
        }
        for (int i = 0; i < r5.length; i++)
        {
            b.put(off + 0x149 + comment.length + i, r5[i]);
        }
        b.putInt(off + 0x1a4, days);
        b.putInt(off + 0x1a8, mins);
        b.putInt(off + 0x1ac, ticks);
        b.put(off + 0x1b0, nameLen);
        for (int i = 0; i < dirName.length; i++)
        {
            b.put(off + 0x1b1 + i, dirName[i]);
        }
        b.putInt(off + 0x1d0, r6);
        b.putInt(off + 0x1d4, real);
        b.putInt(off + 0x1d8, nextLink);
        for (int i = 0; i < 5; i++)
        {
            b.putInt(off + 0x1dc + i * 4, r7[i]);
        }
        b.putInt(off + 0x1f0, nextSameHash);
        b.putInt(off + 0x1f4, parent);
        b.putInt(off + 0x1f8, extension);
        b.putInt(off + 0x1fc, secType);
    }

    public static final int BLOCK_SIZE = 512;
}
