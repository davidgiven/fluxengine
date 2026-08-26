/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  hd_blk.h — bFSHDblock
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

package com.github.adflib.adflib;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * {@code struct bFSHDblock}.
 *
 * <pre>
 * struct bFSHDblock
 * {
 *     char id[4];   // FSHD
 *     int32_t size; // 64
 *     ULONG checksum;
 *     int32_t hostID; // 7
 *     int32_t next;
 *     int32_t flags;
 *     int32_t r1[2];
 *     char dosType[4];
 *     short majVersion;
 *     short minVersion;
 *     int32_t patchFlags;
 *     int32_t type;
 *     int32_t task;
 *     int32_t lock;
 *     int32_t handler;
 *     int32_t stackSize;
 *     int32_t priority;
 *     int32_t startup;
 *     int32_t segListBlock;
 *     int32_t globalVec;
 *     int32_t r2[23];
 *     int32_t r3[21];
 * };
 * </pre>
 */
public final class BFSHDBlock
{

    /*000*/ public byte[] id = new byte[4];   /* FSHD */
    /*004*/ public int size; /* 64 */
    /*008*/ public long checksum; /* ULONG */
    /*00c*/ public int hostID; /* 7 */
    /*010*/ public int next;
    /*014*/ public int flags;
    /*018*/ public int[] r1 = new int[2];
    /*020*/ public byte[] dosType = new byte[4];
    /*024*/ public short majVersion;
    /*026*/ public short minVersion;
    /*028*/ public int patchFlags;
    /*02c*/ public int type;
    /*030*/ public int task;
    /*034*/ public int lock;
    /*038*/ public int handler;
    /*03c*/ public int stackSize;
    /*040*/ public int priority;
    /*044*/ public int startup;
    /*048*/ public int segListBlock;
    /*04c*/ public int globalVec;
    /*050*/ public int[] r2 = new int[23];
    /*0ac*/ public int[] r3 = new int[21];

    public BFSHDBlock()
    {
    }

    public static BFSHDBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BFSHDBlock blk = new BFSHDBlock();
        for (int i = 0; i < 4; i++)
            blk.id[i] = b.get(off + i);
        blk.size = b.getInt(off + 4);
        blk.checksum = b.getInt(off + 8) & 0xFFFFFFFFL;
        blk.hostID = b.getInt(off + 12);
        blk.next = b.getInt(off + 16);
        blk.flags = b.getInt(off + 20);
        blk.r1[0] = b.getInt(off + 24);
        blk.r1[1] = b.getInt(off + 28);
        for (int i = 0; i < 4; i++)
            blk.dosType[i] = b.get(off + 32 + i);
        blk.majVersion = b.getShort(off + 36);
        blk.minVersion = b.getShort(off + 38);
        blk.patchFlags = b.getInt(off + 40);
        blk.type = b.getInt(off + 44);
        blk.task = b.getInt(off + 48);
        blk.lock = b.getInt(off + 52);
        blk.handler = b.getInt(off + 56);
        blk.stackSize = b.getInt(off + 60);
        blk.priority = b.getInt(off + 64);
        blk.startup = b.getInt(off + 68);
        blk.segListBlock = b.getInt(off + 72);
        blk.globalVec = b.getInt(off + 76);
        for (int i = 0; i < 23; i++)
            blk.r2[i] = b.getInt(off + 80 + i * 4);
        for (int i = 0; i < 21; i++)
            blk.r3[i] = b.getInt(off + 172 + i * 4);
        return blk;
    }

    public void write(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        for (int i = 0; i < 4; i++)
            b.put(off + i, id[i]);
        b.putInt(off + 4, size);
        b.putInt(off + 8, (int) (checksum & 0xFFFFFFFFL));
        b.putInt(off + 12, hostID);
        b.putInt(off + 16, next);
        b.putInt(off + 20, flags);
        b.putInt(off + 24, r1[0]);
        b.putInt(off + 28, r1[1]);
        for (int i = 0; i < 4; i++)
            b.put(off + 32 + i, dosType[i]);
        b.putShort(off + 36, majVersion);
        b.putShort(off + 38, minVersion);
        b.putInt(off + 40, patchFlags);
        b.putInt(off + 44, type);
        b.putInt(off + 48, task);
        b.putInt(off + 52, lock);
        b.putInt(off + 56, handler);
        b.putInt(off + 60, stackSize);
        b.putInt(off + 64, priority);
        b.putInt(off + 68, startup);
        b.putInt(off + 72, segListBlock);
        b.putInt(off + 76, globalVec);
        for (int i = 0; i < 23; i++)
            b.putInt(off + 80 + i * 4, r2[i]);
        for (int i = 0; i < 21; i++)
            b.putInt(off + 172 + i * 4, r3[i]);
    }

    public static final int BLOCK_SIZE = 512;
}
