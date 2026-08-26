/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  hd_blk.h — bPARTblock
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
 * {@code struct bPARTblock}.
 *
 * <pre>
 * struct bPARTblock
 * {
 *     char id[4];   // PART
 *     int32_t size; // 64 int32_ts
 *     ULONG checksum;
 *     int32_t hostID; // 7
 *     int32_t next;
 *     int32_t flags;
 *     int32_t r1[2];
 *     int32_t devFlags;
 *     char nameLen;
 *     char name[31];
 *     int32_t r2[15];
 *     int32_t vectorSize; // often 16 int32_ts
 *     int32_t blockSize;  // 128 int32_ts
 *     int32_t secOrg;
 *     int32_t surfaces;
 *     int32_t sectorsPerBlock; // == 1
 *     int32_t blocksPerTrack;
 *     int32_t dosReserved;
 *     int32_t dosPreAlloc;
 *     int32_t interleave;
 *     int32_t lowCyl;
 *     int32_t highCyl;
 *     int32_t numBuffer;
 *     int32_t bufMemType;
 *     int32_t maxTransfer;
 *     int32_t mask;
 *     int32_t bootPri;
 *     char dosType[4];
 *     int32_t r3[15];
 * };
 * </pre>
 */
public final class BPARTBlock {

    /*000*/ public byte[] id = new byte[4];   /* PART */
    /*004*/ public int size; /* 64 int32_ts */
    /*008*/ public long checksum; /* ULONG */
    /*00c*/ public int hostID; /* 7 */
    /*010*/ public int next;
    /*014*/ public int flags;
    /*018*/ public int[] r1 = new int[2];
    /*020*/ public int devFlags;
    /*024*/ public byte nameLen;
    /*025*/ public byte[] name = new byte[31];
    /*044*/ public int[] r2 = new int[15];
    /*080*/ public int vectorSize; /* often 16 int32_ts */
    /*084*/ public int blockSize;  /* 128 int32_ts */
    /*088*/ public int secOrg;
    /*08c*/ public int surfaces;
    /*090*/ public int sectorsPerBlock; /* == 1 */
    /*094*/ public int blocksPerTrack;
    /*098*/ public int dosReserved;
    /*09c*/ public int dosPreAlloc;
    /*0a0*/ public int interleave;
    /*0a4*/ public int lowCyl;
    /*0a8*/ public int highCyl;
    /*0ac*/ public int numBuffer;
    /*0b0*/ public int bufMemType;
    /*0b4*/ public int maxTransfer;
    /*0b8*/ public int mask;
    /*0bc*/ public int bootPri;
    /*0c0*/ public byte[] dosType = new byte[4];
    /*0c4*/ public int[] r3 = new int[15];

    public BPARTBlock() {
    }

    public static BPARTBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BPARTBlock blk = new BPARTBlock();
        for (int i = 0; i < 4; i++) blk.id[i] = b.get(off + i);
        blk.size = b.getInt(off + 4);
        blk.checksum = b.getInt(off + 8) & 0xFFFFFFFFL;
        blk.hostID = b.getInt(off + 12);
        blk.next = b.getInt(off + 16);
        blk.flags = b.getInt(off + 20);
        blk.r1[0] = b.getInt(off + 24);
        blk.r1[1] = b.getInt(off + 28);
        blk.devFlags = b.getInt(off + 32);
        blk.nameLen = b.get(off + 36);
        for (int i = 0; i < 31; i++) blk.name[i] = b.get(off + 37 + i);
        for (int i = 0; i < 15; i++) blk.r2[i] = b.getInt(off + 68 + i * 4);
        blk.vectorSize = b.getInt(off + 0x80);
        blk.blockSize = b.getInt(off + 0x84);
        blk.secOrg = b.getInt(off + 0x88);
        blk.surfaces = b.getInt(off + 0x8c);
        blk.sectorsPerBlock = b.getInt(off + 0x90);
        blk.blocksPerTrack = b.getInt(off + 0x94);
        blk.dosReserved = b.getInt(off + 0x98);
        blk.dosPreAlloc = b.getInt(off + 0x9c);
        blk.interleave = b.getInt(off + 0xa0);
        blk.lowCyl = b.getInt(off + 0xa4);
        blk.highCyl = b.getInt(off + 0xa8);
        blk.numBuffer = b.getInt(off + 0xac);
        blk.bufMemType = b.getInt(off + 0xb0);
        blk.maxTransfer = b.getInt(off + 0xb4);
        blk.mask = b.getInt(off + 0xb8);
        blk.bootPri = b.getInt(off + 0xbc);
        for (int i = 0; i < 4; i++) blk.dosType[i] = b.get(off + 0xc0 + i);
        for (int i = 0; i < 15; i++) blk.r3[i] = b.getInt(off + 0xc4 + i * 4);
        return blk;
    }

    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        for (int i = 0; i < 4; i++) b.put(off + i, id[i]);
        b.putInt(off + 4, size);
        b.putInt(off + 8, (int) (checksum & 0xFFFFFFFFL));
        b.putInt(off + 12, hostID);
        b.putInt(off + 16, next);
        b.putInt(off + 20, flags);
        b.putInt(off + 24, r1[0]);
        b.putInt(off + 28, r1[1]);
        b.putInt(off + 32, devFlags);
        b.put(off + 36, nameLen);
        for (int i = 0; i < 31; i++) b.put(off + 37 + i, name[i]);
        for (int i = 0; i < 15; i++) b.putInt(off + 68 + i * 4, r2[i]);
        b.putInt(off + 0x80, vectorSize);
        b.putInt(off + 0x84, blockSize);
        b.putInt(off + 0x88, secOrg);
        b.putInt(off + 0x8c, surfaces);
        b.putInt(off + 0x90, sectorsPerBlock);
        b.putInt(off + 0x94, blocksPerTrack);
        b.putInt(off + 0x98, dosReserved);
        b.putInt(off + 0x9c, dosPreAlloc);
        b.putInt(off + 0xa0, interleave);
        b.putInt(off + 0xa4, lowCyl);
        b.putInt(off + 0xa8, highCyl);
        b.putInt(off + 0xac, numBuffer);
        b.putInt(off + 0xb0, bufMemType);
        b.putInt(off + 0xb4, maxTransfer);
        b.putInt(off + 0xb8, mask);
        b.putInt(off + 0xbc, bootPri);
        for (int i = 0; i < 4; i++) b.put(off + 0xc0 + i, dosType[i]);
        for (int i = 0; i < 15; i++) b.putInt(off + 0xc4 + i * 4, r3[i]);
    }

    public static final int BLOCK_SIZE = 512;
}
