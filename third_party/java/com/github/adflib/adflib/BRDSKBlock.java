/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  hd_blk.h — bRDSKblock
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
 * {@code struct bRDSKblock}.
 *
 * <pre>
 * struct bRDSKblock
 * {
 *     char id[4];   // RDSK
 *     int32_t size; // 64 int32_ts
 *     ULONG checksum;
 *     int32_t hostID;    // 7
 *     int32_t blockSize; // 512 bytes
 *     int32_t flags;     // 0x17
 *     int32_t badBlockList;
 *     int32_t partitionList;
 *     int32_t fileSysHdrList;
 *     int32_t driveInit;
 *     int32_t r1[6]; // -1
 *     int32_t cylinders;
 *     int32_t sectors;
 *     int32_t heads;
 *     int32_t interleave;
 *     int32_t parkingZone;
 *     int32_t r2[3]; // 0
 *     int32_t writePreComp;
 *     int32_t reducedWrite;
 *     int32_t stepRate;
 *     int32_t r3[5]; // 0
 *     int32_t rdbBlockLo;
 *     int32_t rdbBlockHi;
 *     int32_t loCylinder;
 *     int32_t hiCylinder;
 *     int32_t cylBlocks;
 *     int32_t autoParkSeconds;
 *     int32_t highRDSKBlock;
 *     int32_t r4; // 0
 *     char diskVendor[8];
 *     char diskProduct[16];
 *     char diskRevision[4];
 *     char controllerVendor[8];
 *     char controllerProduct[16];
 *     char controllerRevision[4];
 *     int32_t r5[10]; // 0
 * };
 * </pre>
 */
public final class BRDSKBlock
{

    /*000*/ public byte[] id = new byte[4];   /* RDSK */
    /*004*/ public int size; /* 64 int32_ts */
    /*008*/ public long checksum; /* ULONG */
    /*00c*/ public int hostID;    /* 7 */
    /*010*/ public int blockSize; /* 512 bytes */
    /*014*/ public int flags;     /* 0x17 */
    /*018*/ public int badBlockList;
    /*01c*/ public int partitionList;
    /*020*/ public int fileSysHdrList;
    /*024*/ public int driveInit;
    /*028*/ public int[] r1 = new int[6]; /* -1 */
    /*040*/ public int cylinders;
    /*044*/ public int sectors;
    /*048*/ public int heads;
    /*04c*/ public int interleave;
    /*050*/ public int parkingZone;
    /*054*/ public int[] r2 = new int[3]; /* 0 */
    /*060*/ public int writePreComp;
    /*064*/ public int reducedWrite;
    /*068*/ public int stepRate;
    /*06c*/ public int[] r3 = new int[5]; /* 0 */
    /*080*/ public int rdbBlockLo;
    /*084*/ public int rdbBlockHi;
    /*088*/ public int loCylinder;
    /*08c*/ public int hiCylinder;
    /*090*/ public int cylBlocks;
    /*094*/ public int autoParkSeconds;
    /*098*/ public int highRDSKBlock;
    /*09c*/ public int r4; /* 0 */
    /*0a0*/ public byte[] diskVendor = new byte[8];
    /*0a8*/ public byte[] diskProduct = new byte[16];
    /*0b8*/ public byte[] diskRevision = new byte[4];
    /*0bc*/ public byte[] controllerVendor = new byte[8];
    /*0c4*/ public byte[] controllerProduct = new byte[16];
    /*0d4*/ public byte[] controllerRevision = new byte[4];
    /*0d8*/ public int[] r5 = new int[10]; /* 0 */

    public BRDSKBlock()
    {
    }

    public static BRDSKBlock read(ByteBuffer buf, int off)
    {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BRDSKBlock blk = new BRDSKBlock();
        for (int i = 0; i < 4; i++)
            blk.id[i] = b.get(off + i);
        blk.size = b.getInt(off + 4);
        blk.checksum = b.getInt(off + 8) & 0xFFFFFFFFL;
        blk.hostID = b.getInt(off + 12);
        blk.blockSize = b.getInt(off + 16);
        blk.flags = b.getInt(off + 20);
        blk.badBlockList = b.getInt(off + 24);
        blk.partitionList = b.getInt(off + 28);
        blk.fileSysHdrList = b.getInt(off + 32);
        blk.driveInit = b.getInt(off + 36);
        for (int i = 0; i < 6; i++)
            blk.r1[i] = b.getInt(off + 40 + i * 4);
        blk.cylinders = b.getInt(off + 0x40);
        blk.sectors = b.getInt(off + 0x44);
        blk.heads = b.getInt(off + 0x48);
        blk.interleave = b.getInt(off + 0x4c);
        blk.parkingZone = b.getInt(off + 0x50);
        for (int i = 0; i < 3; i++)
            blk.r2[i] = b.getInt(off + 0x54 + i * 4);
        blk.writePreComp = b.getInt(off + 0x60);
        blk.reducedWrite = b.getInt(off + 0x64);
        blk.stepRate = b.getInt(off + 0x68);
        for (int i = 0; i < 5; i++)
            blk.r3[i] = b.getInt(off + 0x6c + i * 4);
        blk.rdbBlockLo = b.getInt(off + 0x80);
        blk.rdbBlockHi = b.getInt(off + 0x84);
        blk.loCylinder = b.getInt(off + 0x88);
        blk.hiCylinder = b.getInt(off + 0x8c);
        blk.cylBlocks = b.getInt(off + 0x90);
        blk.autoParkSeconds = b.getInt(off + 0x94);
        blk.highRDSKBlock = b.getInt(off + 0x98);
        blk.r4 = b.getInt(off + 0x9c);
        for (int i = 0; i < 8; i++)
            blk.diskVendor[i] = b.get(off + 0xa0 + i);
        for (int i = 0; i < 16; i++)
            blk.diskProduct[i] = b.get(off + 0xa8 + i);
        for (int i = 0; i < 4; i++)
            blk.diskRevision[i] = b.get(off + 0xb8 + i);
        for (int i = 0; i < 8; i++)
            blk.controllerVendor[i] = b.get(off + 0xbc + i);
        for (int i = 0; i < 16; i++)
            blk.controllerProduct[i] = b.get(off + 0xc4 + i);
        for (int i = 0; i < 4; i++)
            blk.controllerRevision[i] = b.get(off + 0xd4 + i);
        for (int i = 0; i < 10; i++)
            blk.r5[i] = b.getInt(off + 0xd8 + i * 4);
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
        b.putInt(off + 16, blockSize);
        b.putInt(off + 20, flags);
        b.putInt(off + 24, badBlockList);
        b.putInt(off + 28, partitionList);
        b.putInt(off + 32, fileSysHdrList);
        b.putInt(off + 36, driveInit);
        for (int i = 0; i < 6; i++)
            b.putInt(off + 40 + i * 4, r1[i]);
        b.putInt(off + 0x40, cylinders);
        b.putInt(off + 0x44, sectors);
        b.putInt(off + 0x48, heads);
        b.putInt(off + 0x4c, interleave);
        b.putInt(off + 0x50, parkingZone);
        for (int i = 0; i < 3; i++)
            b.putInt(off + 0x54 + i * 4, r2[i]);
        b.putInt(off + 0x60, writePreComp);
        b.putInt(off + 0x64, reducedWrite);
        b.putInt(off + 0x68, stepRate);
        for (int i = 0; i < 5; i++)
            b.putInt(off + 0x6c + i * 4, r3[i]);
        b.putInt(off + 0x80, rdbBlockLo);
        b.putInt(off + 0x84, rdbBlockHi);
        b.putInt(off + 0x88, loCylinder);
        b.putInt(off + 0x8c, hiCylinder);
        b.putInt(off + 0x90, cylBlocks);
        b.putInt(off + 0x94, autoParkSeconds);
        b.putInt(off + 0x98, highRDSKBlock);
        b.putInt(off + 0x9c, r4);
        for (int i = 0; i < 8; i++)
            b.put(off + 0xa0 + i, diskVendor[i]);
        for (int i = 0; i < 16; i++)
            b.put(off + 0xa8 + i, diskProduct[i]);
        for (int i = 0; i < 4; i++)
            b.put(off + 0xb8 + i, diskRevision[i]);
        for (int i = 0; i < 8; i++)
            b.put(off + 0xbc + i, controllerVendor[i]);
        for (int i = 0; i < 16; i++)
            b.put(off + 0xc4 + i, controllerProduct[i]);
        for (int i = 0; i < 4; i++)
            b.put(off + 0xd4 + i, controllerRevision[i]);
        for (int i = 0; i < 10; i++)
            b.putInt(off + 0xd8 + i * 4, r5[i]);
    }

    public static final int BLOCK_SIZE = 512;
}
