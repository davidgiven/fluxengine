/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_blk.h — bRootBlock
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
 * {@code struct bRootBlock} — 512-byte root block.
 *
 * <pre>
 * struct bRootBlock
 * {
 *     int32_t type;
 *     int32_t headerKey;
 *     int32_t highSeq;
 *     int32_t hashTableSize;
 *     int32_t firstData;
 *     ULONG checkSum;
 *     int32_t hashTable[HT_SIZE];
 *     int32_t bmFlag;
 *     int32_t bmPages[BM_SIZE];
 *     int32_t bmExt;
 *     int32_t cDays;
 *     int32_t cMins;
 *     int32_t cTicks;
 *     char nameLen;
 *     char diskName[MAXNAMELEN + 1];
 *     char r2[8];
 *     int32_t days;
 *     int32_t mins;
 *     int32_t ticks;
 *     int32_t coDays;
 *     int32_t coMins;
 *     int32_t coTicks;
 *     int32_t nextSameHash;
 *     int32_t parent;
 *     int32_t extension;
 *     int32_t secType;
 * };
 * </pre>
 */
public final class BRootBlock {

    /*000*/ public int type;
    public int headerKey;
    public int highSeq;
    /*00c*/ public int hashTableSize;
    public int firstData;
    /*014*/ public long checkSum; /* ULONG */
    /*018*/ public int[] hashTable = new int[AdfConstants.HT_SIZE]; /* hash table */
    /*138*/ public int bmFlag;             /* bitmap flag, -1 means VALID */
    /*13c*/ public int[] bmPages = new int[AdfConstants.BM_SIZE];
    /*1a0*/ public int bmExt;
    /*1a4*/ public int cDays; /* creation date FFS and OFS */
    /*1a8*/ public int cMins;
    /*1ac*/ public int cTicks;
    /*1b0*/ public byte nameLen;
    /*1b1*/ public byte[] diskName = new byte[AdfConstants.MAXNAMELEN + 1];
    public byte[] r2 = new byte[8];
    /*1d8*/ public int days;   /* last access : days after 1 jan 1978 */
    /*1dc*/ public int mins;   /* hours and minutes in minutes */
    /*1e0*/ public int ticks;  /* 1/50 seconds */
    /*1e4*/ public int coDays; /* creation date OFS */
    /*1e8*/ public int coMins;
    /*1ec*/ public int coTicks;
    public int nextSameHash;      /* == 0 */
    public int parent;            /* == 0 */
    /*1f8*/ public int extension; /* FFS: first directory cache block */
    /*1fc*/ public int secType;   /* == 1 */

    public BRootBlock() {
    }

    /**
     * Reads a {@code bRootBlock} from {@code buf} at absolute offset {@code off}.
     */
    public static BRootBlock read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BRootBlock blk = new BRootBlock();
        blk.type = b.getInt(off + 0);
        blk.headerKey = b.getInt(off + 4);
        blk.highSeq = b.getInt(off + 8);
        blk.hashTableSize = b.getInt(off + 12);
        blk.firstData = b.getInt(off + 16);
        blk.checkSum = b.getInt(off + 20) & 0xFFFFFFFFL;
        for (int i = 0; i < AdfConstants.HT_SIZE; i++) {
            blk.hashTable[i] = b.getInt(off + 24 + i * 4);
        }
        blk.bmFlag = b.getInt(off + 0x138);
        for (int i = 0; i < AdfConstants.BM_SIZE; i++) {
            blk.bmPages[i] = b.getInt(off + 0x13c + i * 4);
        }
        blk.bmExt = b.getInt(off + 0x1a0);
        blk.cDays = b.getInt(off + 0x1a4);
        blk.cMins = b.getInt(off + 0x1a8);
        blk.cTicks = b.getInt(off + 0x1ac);
        blk.nameLen = b.get(off + 0x1b0);
        for (int i = 0; i < blk.diskName.length; i++) {
            blk.diskName[i] = b.get(off + 0x1b1 + i);
        }
        for (int i = 0; i < blk.r2.length; i++) {
            blk.r2[i] = b.get(off + 0x1b1 + blk.diskName.length + i);
        }
        blk.days = b.getInt(off + 0x1d8);
        blk.mins = b.getInt(off + 0x1dc);
        blk.ticks = b.getInt(off + 0x1e0);
        blk.coDays = b.getInt(off + 0x1e4);
        blk.coMins = b.getInt(off + 0x1e8);
        blk.coTicks = b.getInt(off + 0x1ec);
        blk.nextSameHash = b.getInt(off + 0x1f0);
        blk.parent = b.getInt(off + 0x1f4);
        blk.extension = b.getInt(off + 0x1f8);
        blk.secType = b.getInt(off + 0x1fc);
        return blk;
    }

    /**
     * Writes this {@code bRootBlock} to {@code buf} at absolute offset {@code off}.
     */
    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, type);
        b.putInt(off + 4, headerKey);
        b.putInt(off + 8, highSeq);
        b.putInt(off + 12, hashTableSize);
        b.putInt(off + 16, firstData);
        b.putInt(off + 20, (int) (checkSum & 0xFFFFFFFFL));
        for (int i = 0; i < AdfConstants.HT_SIZE; i++) {
            b.putInt(off + 24 + i * 4, hashTable[i]);
        }
        b.putInt(off + 0x138, bmFlag);
        for (int i = 0; i < AdfConstants.BM_SIZE; i++) {
            b.putInt(off + 0x13c + i * 4, bmPages[i]);
        }
        b.putInt(off + 0x1a0, bmExt);
        b.putInt(off + 0x1a4, cDays);
        b.putInt(off + 0x1a8, cMins);
        b.putInt(off + 0x1ac, cTicks);
        b.put(off + 0x1b0, nameLen);
        for (int i = 0; i < diskName.length; i++) {
            b.put(off + 0x1b1 + i, diskName[i]);
        }
        for (int i = 0; i < r2.length; i++) {
            b.put(off + 0x1b1 + diskName.length + i, r2[i]);
        }
        b.putInt(off + 0x1d8, days);
        b.putInt(off + 0x1dc, mins);
        b.putInt(off + 0x1e0, ticks);
        b.putInt(off + 0x1e4, coDays);
        b.putInt(off + 0x1e8, coMins);
        b.putInt(off + 0x1ec, coTicks);
        b.putInt(off + 0x1f0, nextSameHash);
        b.putInt(off + 0x1f4, parent);
        b.putInt(off + 0x1f8, extension);
        b.putInt(off + 0x1fc, secType);
    }

    public static final int BLOCK_SIZE = 512;
}
