/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_raw.c / adf_raw.h + defendian.h
 *
 *  $Id$
 *
 *  blocks level code
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
import java.util.Arrays;

/**
 * Java port of {@code adf_raw.c} / {@code adf_raw.h}.
 *
 * <p>Logical disk / volume block helpers. On-disk structures are big-endian
 * (Motorola 68k). {@code defendian.h} mapped via {@link AdfEndian}; in Java
 * we always handle endian explicitly via {@link ByteBuffer#order}.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment; public API uses Javadoc {@code /**}.
 */
public final class AdfRaw {

    private AdfRaw() {
    }

    /* adf_raw.h */

    public static final int SW_LONG = 4;
    public static final int SW_SHORT = 2;
    public static final int SW_CHAR = 1;

    public static final int MAX_SWTYPE = 11;

    public static final int SWBL_BOOT = 0;
    public static final int SWBL_ROOT = 1;
    public static final int SWBL_DATA = 2;
    public static final int SWBL_FILE = 3;
    public static final int SWBL_ENTRY = 3;
    public static final int SWBL_DIR = 3;
    public static final int SWBL_CACHE = 4;
    public static final int SWBL_BITMAP = 5;
    public static final int SWBL_FEXT = 5;
    public static final int SWBL_LINK = 6;
    public static final int SWBL_BITMAPE = 5;
    public static final int SWBL_RDSK = 7;
    public static final int SWBL_BADB = 8;
    public static final int SWBL_PART = 9;
    public static final int SWBL_FSHD = 10;
    public static final int SWBL_LSEG = 11;

    /* swapTable as in adf_raw.c */

    public static final int[][] swapTable = {
        { 4, SW_CHAR, 2, SW_LONG, 1012, SW_CHAR, 0, 1024 },     /* first bytes of boot */
        { 108, SW_LONG, 40, SW_CHAR, 10, SW_LONG, 0, 512 },        /* root */
        { 6, SW_LONG, 488, SW_CHAR, 0, 512 },                      /* data */
                                                            /* file, dir, entry */
        { 82, SW_LONG, 92, SW_CHAR, 3, SW_LONG, 36, SW_CHAR, 11, SW_LONG, 0, 512 },
        { 6, SW_LONG, 0, 24 },                                       /* cache */
        { 128, SW_LONG, 0, 512 },                                /* bitmap, fext */
                                                                /* link */
        { 6, SW_LONG, 64, SW_CHAR, 86, SW_LONG, 32, SW_CHAR, 12, SW_LONG, 0, 512 },
        { 4, SW_CHAR, 39, SW_LONG, 56, SW_CHAR, 10, SW_LONG, 0, 256 }, /* RDSK */
        { 4, SW_CHAR, 127, SW_LONG, 0, 512 },                          /* BADB */
        { 4, SW_CHAR, 8, SW_LONG, 32, SW_CHAR, 31, SW_LONG, 4, SW_CHAR, /* PART */
          15, SW_LONG, 0, 256 },
        { 4, SW_CHAR, 7, SW_LONG, 4, SW_CHAR, 55, SW_LONG, 0, 256 }, /* FSHD */
        { 4, SW_CHAR, 4, SW_LONG, 492, SW_CHAR, 0, 512 }             /* LSEG */
    };

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = new Env();

    /*
     * swapEndian
     *
     * magic :-) endian swap function (big -> little for read, little to big for write)
     */

    public static void swapEndian(byte[] buf, int type) {
        int i = 0;
        int j = 0;
        int p = 0;

        i = 0;
        p = 0;

        if (type > MAX_SWTYPE || type < 0) {
            if (adfEnv != null && adfEnv.eFct != null) {
                adfEnv.eFct.call("SwapEndian: type do not exist");
            }
        }

        while (swapTable[type][i] != 0) {
            for (j = 0; j < swapTable[type][i]; j++) {
                switch (swapTable[type][i + 1]) {
                case SW_LONG:
                    {
                        int b0 = buf[p] & 0xFF;
                        int b1 = buf[p + 1] & 0xFF;
                        int b2 = buf[p + 2] & 0xFF;
                        int b3 = buf[p + 3] & 0xFF;
                        buf[p] = (byte) b3;
                        buf[p + 1] = (byte) b2;
                        buf[p + 2] = (byte) b1;
                        buf[p + 3] = (byte) b0;
                        p += 4;
                    }
                    break;
                case SW_SHORT:
                    {
                        int b0 = buf[p] & 0xFF;
                        int b1 = buf[p + 1] & 0xFF;
                        buf[p] = (byte) b1;
                        buf[p + 1] = (byte) b0;
                        p += 2;
                    }
                    break;
                case SW_CHAR:
                    p++;
                    break;
                default:
                    ;
                }
            }
            i += 2;
        }
        if (p != swapTable[type][i + 1]) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("Warning: Endian Swapping length");
            }
        }
    }

    /**
     * ByteBuffer overload — swaps in-place at absolute offset {@code off}
     * without touching position/limit.
     */
    public static void swapEndian(ByteBuffer buf, int off, int type) {
        byte[] tmp = new byte[swapTable[type][swapTable[type].length - 1]];
        // copy needed range
        int total = swapTable[type][swapTable[type].length - 1];
        for (int k = 0; k < total; k++) {
            tmp[k] = buf.get(off + k);
        }
        swapEndian(tmp, type);
        for (int k = 0; k < total; k++) {
            buf.put(off + k, tmp[k]);
        }
    }

    private static void swLong(byte[] buf, int off, long val) {
        buf[off] = (byte) ((val >> 24) & 0xFF);
        buf[off + 1] = (byte) ((val >> 16) & 0xFF);
        buf[off + 2] = (byte) ((val >> 8) & 0xFF);
        buf[off + 3] = (byte) (val & 0xFF);
    }

    private static void swLong(ByteBuffer buf, int off, long val) {
        buf.put(off, (byte) ((val >> 24) & 0xFF));
        buf.put(off + 1, (byte) ((val >> 16) & 0xFF));
        buf.put(off + 2, (byte) ((val >> 8) & 0xFF));
        buf.put(off + 3, (byte) (val & 0xFF));
    }

    private static void copyRoot(BRootBlock src, BRootBlock dst) {
        dst.type = src.type;
        dst.headerKey = src.headerKey;
        dst.highSeq = src.highSeq;
        dst.hashTableSize = src.hashTableSize;
        dst.firstData = src.firstData;
        dst.checkSum = src.checkSum;
        System.arraycopy(src.hashTable, 0, dst.hashTable, 0, src.hashTable.length);
        dst.bmFlag = src.bmFlag;
        System.arraycopy(src.bmPages, 0, dst.bmPages, 0, src.bmPages.length);
        dst.bmExt = src.bmExt;
        dst.cDays = src.cDays;
        dst.cMins = src.cMins;
        dst.cTicks = src.cTicks;
        dst.nameLen = src.nameLen;
        System.arraycopy(src.diskName, 0, dst.diskName, 0, src.diskName.length);
        System.arraycopy(src.r2, 0, dst.r2, 0, src.r2.length);
        dst.days = src.days;
        dst.mins = src.mins;
        dst.ticks = src.ticks;
        dst.coDays = src.coDays;
        dst.coMins = src.coMins;
        dst.coTicks = src.coTicks;
        dst.nextSameHash = src.nextSameHash;
        dst.parent = src.parent;
        dst.extension = src.extension;
        dst.secType = src.secType;
    }

    /*
     * adfReadRootBlock
     *
     * ENDIAN DEPENDENT
     */

    public static AdfError adfReadRootBlock(Volume vol, int nSect, BRootBlock root) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BRootBlock tmp = BRootBlock.read(bb, 0);
        copyRoot(tmp, root);

        if (root.type != AdfConstants.T_HEADER || root.secType != AdfConstants.ST_ROOT) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadRootBlock : id not found");
            }
            return AdfError.RC_ERROR;
        }
        if (root.checkSum != adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadRootBlock : invalid checksum");
            }
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteRootBlock
     *
     * 
     */

    public static AdfError adfWriteRootBlock(Volume vol, int nSect, BRootBlock root) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;

        root.type = AdfConstants.T_HEADER;
        root.headerKey = 0;
        root.highSeq = 0;
        root.hashTableSize = AdfConstants.HT_SIZE;
        root.firstData = 0;
        /* checkSum, hashTable */
        /* bmflag */
        /* bmPages, bmExt */
        root.nextSameHash = 0;
        root.parent = 0;
        root.secType = AdfConstants.ST_ROOT;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        root.write(bb, 0);

        newSum = adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE);
        swLong(buf, 20, newSum);

        if (AdfDisk.adfWriteBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        return AdfError.RC_OK;
    }

    /*
     * adfReadBootBlock
     *
     * ENDIAN DEPENDENT
     */

    public static AdfError adfReadBootBlock(Volume vol, BBootBlock boot) {
        byte[] buf = new byte[1024];

        if (AdfDisk.adfReadBlock(vol, 0, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        // second logical block at buf+512
        byte[] buf1 = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        if (AdfDisk.adfReadBlock(vol, 1, buf1) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        System.arraycopy(buf1, 0, buf, AdfConstants.LOGICAL_BLOCK_SIZE, AdfConstants.LOGICAL_BLOCK_SIZE);

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BBootBlock tmp = BBootBlock.read(bb, 0);
        // copy into boot
        System.arraycopy(tmp.dosType, 0, boot.dosType, 0, 4);
        boot.checkSum = tmp.checkSum;
        boot.rootBlock = tmp.rootBlock;
        System.arraycopy(tmp.data, 0, boot.data, 0, tmp.data.length);

        if (boot.dosType[0] != 'D' || boot.dosType[1] != 'O' || boot.dosType[2] != 'S') {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadBootBlock : DOS id not found");
            }
            return AdfError.RC_ERROR;
        }

        if (boot.data[0] != 0 && adfBootSum(buf) != boot.checkSum) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadBootBlock : incorrect checksum");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteBootBlock
     *
     *
     *     write bootcode ?
     */

    public static AdfError adfWriteBootBlock(Volume vol, BBootBlock boot) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE * 2];
        long newSum = 0;

        boot.dosType[0] = 'D';
        boot.dosType[1] = 'O';
        boot.dosType[2] = 'S';
        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        boot.write(bb, 0);

        if (boot.rootBlock == 880 || boot.data[0] != 0) {
            newSum = adfBootSum(buf);
            swLong(buf, 4, newSum);
        }

        byte[] first = Arrays.copyOfRange(buf, 0, AdfConstants.LOGICAL_BLOCK_SIZE);
        byte[] second = Arrays.copyOfRange(buf, AdfConstants.LOGICAL_BLOCK_SIZE, AdfConstants.LOGICAL_BLOCK_SIZE * 2);
        if (AdfDisk.adfWriteBlock(vol, 0, first) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        if (AdfDisk.adfWriteBlock(vol, 1, second) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        return AdfError.RC_OK;
    }

    /*
     * NormalSum
     *
     * buf = where the block is stored
     * offset = checksum place (in bytes)
     * bufLen = buffer length (in bytes)
     */

    public static long adfNormalSum(byte[] buf, int offset, int bufLen) {
        long newsum = 0;
        int i = 0;

        newsum = 0L;
        for (i = 0; i < (bufLen / 4); i++) {
            if (i != (offset / 4)) {
                newsum += AdfEndian.Long(buf, i * 4) & 0xFFFFFFFFL;
            }
        }
        newsum = (-newsum) & 0xFFFFFFFFL;

        return newsum;
    }

    /** ByteBuffer overload — reads big-endian, absolute, without position side-effects. */
    public static long adfNormalSum(ByteBuffer buf, int offset, int bufLen) {
        long newsum = 0L;
        int i = 0;
        for (i = 0; i < (bufLen / 4); i++) {
            if (i != (offset / 4)) {
                newsum += buf.getInt(i * 4) & 0xFFFFFFFFL;
            }
        }
        newsum = (-newsum) & 0xFFFFFFFFL;
        return newsum;
    }

    /*
     * adfBitmapSum
     *
     */

    public static long adfBitmapSum(byte[] buf) {
        long newSum = 0L;
        int i = 0;

        newSum = 0L;
        for (i = 1; i < 128; i++) {
            newSum -= AdfEndian.Long(buf, i * 4) & 0xFFFFFFFFL;
            newSum &= 0xFFFFFFFFL;
        }
        return newSum & 0xFFFFFFFFL;
    }

    /*
     * adfBootSum
     *
     */

    public static long adfBootSum(byte[] buf) {
        long d = 0;
        long newSum = 0L;
        int i = 0;

        newSum = 0L;
        for (i = 0; i < 256; i++) {
            if (i != 1) {
                d = AdfEndian.Long(buf, i * 4) & 0xFFFFFFFFL;
                if ((0xFFFFFFFFL - newSum) < d) {
                    newSum++;
                    newSum &= 0xFFFFFFFFL;
                }
                newSum += d;
                newSum &= 0xFFFFFFFFL;
            }
        }
        newSum = (~newSum) & 0xFFFFFFFFL;

        return newSum;
    }

    public static long adfBootSum2(byte[] buf) {
        long prevsum = 0L;
        long newSum = 0L;
        int i = 0;

        prevsum = 0L;
        newSum = 0L;
        for (i = 0; i < 1024 / 4; i++) {
            if (i != 1) {
                prevsum = newSum;
                newSum += AdfEndian.Long(buf, i * 4) & 0xFFFFFFFFL;
                newSum &= 0xFFFFFFFFL;
                if (newSum < prevsum) {
                    newSum++;
                    newSum &= 0xFFFFFFFFL;
                }
            }
        }
        newSum = (~newSum) & 0xFFFFFFFFL;

        return newSum;
    }

    /** ByteBuffer overloads for sums — absolute big-endian. */
    public static long adfBitmapSum(ByteBuffer buf) {
        long newSum = 0L;
        for (int i = 1; i < 128; i++) {
            newSum -= buf.getInt(i * 4) & 0xFFFFFFFFL;
            newSum &= 0xFFFFFFFFL;
        }
        return newSum & 0xFFFFFFFFL;
    }

    public static long adfBootSum(ByteBuffer buf) {
        long newSum = 0L;
        for (int i = 0; i < 256; i++) {
            if (i != 1) {
                long d = buf.getInt(i * 4) & 0xFFFFFFFFL;
                if ((0xFFFFFFFFL - newSum) < d) {
                    newSum = (newSum + 1) & 0xFFFFFFFFL;
                }
                newSum = (newSum + d) & 0xFFFFFFFFL;
            }
        }
        return (~newSum) & 0xFFFFFFFFL;
    }
}
