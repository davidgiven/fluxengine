/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_bitm.c / adf_bitm.h
 *
 *  $Id$
 *
 *  bitmap code
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
 * Java port of {@code adf_bitm.c} / {@code adf_bitm.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data.
 * High-level objects use normal Java classes. Return codes use {@link AdfError}.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfBitm
{

    private AdfBitm()
    {
    }

    /**
     * Global environment — mirrors {@code extern struct Env adfEnv}.
     */
    public static Env adfEnv = AdfRaw.adfEnv;

    public static final long[] bitMask = {0x1L,
            0x2L,
            0x4L,
            0x8L,
            0x10L,
            0x20L,
            0x40L,
            0x80L,
            0x100L,
            0x200L,
            0x400L,
            0x800L,
            0x1000L,
            0x2000L,
            0x4000L,
            0x8000L,
            0x10000L,
            0x20000L,
            0x40000L,
            0x80000L,
            0x100000L,
            0x200000L,
            0x400000L,
            0x800000L,
            0x1000000L,
            0x2000000L,
            0x4000000L,
            0x8000000L,
            0x10000000L,
            0x20000000L,
            0x40000000L,
            0x80000000L};

    /*
     * adfUpdateBitmap
     *
     */

    public static AdfError adfUpdateBitmap(Volume vol)
    {
        int i = 0;
        BRootBlock root = new BRootBlock();

        if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        root.bmFlag = AdfConstants.BM_INVALID;
        if (AdfRaw.adfWriteRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        for (i = 0; i < vol.bitmapSize; i++)
        {
            if (vol.bitmapBlocksChg[i])
            {
                if (adfWriteBitmapBlock(vol, vol.bitmapBlocks[i], vol.bitmapTable[i]) !=
                        AdfError.RC_OK)
                {
                    return AdfError.RC_ERROR;
                }
                vol.bitmapBlocksChg[i] = false;
            }
        }

        root.bmFlag = AdfConstants.BM_VALID;
        int[] day = new int[1];
        int[] min = new int[1];
        int[] ticks = new int[1];
        adfTime2AmigaTime(adfGiveCurrentTime(), day, min, ticks);
        root.days = day[0];
        root.mins = min[0];
        root.ticks = ticks[0];
        if (AdfRaw.adfWriteRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCountFreeBlocks
     *
     */

    public static int adfCountFreeBlocks(Volume vol)
    {
        if (vol.bitmapTable == null || vol.bitmapBlocks == null)
        {
            return 0;
        }
        int freeBlocks = 0;
        int j = 0;

        freeBlocks = 0;
        for (j = vol.firstBlock + 2; j <= (vol.lastBlock - vol.firstBlock); j++)
        {
            if (adfIsBlockFree(vol, j))
            {
                freeBlocks++;
            }
        }

        return freeBlocks;
    }

    /*
     * adfReadBitmap
     *
     */

    public static AdfError adfReadBitmap(Volume vol, int nBlock, BRootBlock root)
    {
        int mapSize = 0;
        int nSect = 0;
        int j = 0;
        int i = 0;
        BBitmapExtBlock bmExt = new BBitmapExtBlock();

        mapSize = nBlock / (127 * 32);
        if ((nBlock % (127 * 32)) != 0)
        {
            mapSize++;
        }
        vol.bitmapSize = mapSize;

        vol.bitmapTable = new BBitmapBlock[mapSize];
        if (vol.bitmapTable == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfReadBitmap : malloc, vol->bitmapTable");
            }
            return AdfError.RC_MALLOC;
        }
        vol.bitmapBlocks = new int[mapSize];
        if (vol.bitmapBlocks == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfReadBitmap : malloc, vol->bitmapBlocks");
            }
            return AdfError.RC_MALLOC;
        }
        vol.bitmapBlocksChg = new boolean[mapSize];
        if (vol.bitmapBlocksChg == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfReadBitmap : malloc, vol->bitmapBlocks");
            }
            return AdfError.RC_MALLOC;
        }
        for (i = 0; i < mapSize; i++)
        {
            vol.bitmapBlocksChg[i] = false;

            vol.bitmapTable[i] = new BBitmapBlock();
            if (vol.bitmapTable[i] == null)
            {
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfReadBitmap : malloc, vol->bitmapBlocks");
                }
                return AdfError.RC_MALLOC;
            }
        }

        j = 0;
        i = 0;
        /* bitmap pointers in rootblock : 0 <= i <BM_SIZE */
        while (i < AdfConstants.BM_SIZE && root.bmPages[i] != 0)
        {
            vol.bitmapBlocks[j] = nSect = root.bmPages[i];
            if (!isSectNumValid(vol, nSect))
            {
                if (adfEnv != null && adfEnv.wFct != null)
                {
                    adfEnv.wFct.call("adfReadBitmap : sector out of range");
                }
            }

            if (adfReadBitmapBlock(vol, nSect, vol.bitmapTable[j]) != AdfError.RC_OK)
            {
                adfFreeBitmap(vol);
                return AdfError.RC_ERROR;
            }
            j++;
            i++;
        }
        nSect = root.bmExt;
        while (nSect != 0)
        {
            /* bitmap pointers in bitmapExtBlock, j <= mapSize */
            if (adfReadBitmapExtBlock(vol, nSect, bmExt) != AdfError.RC_OK)
            {
                adfFreeBitmap(vol);
                return AdfError.RC_ERROR;
            }
            i = 0;
            while (i < 127 && j < mapSize)
            {
                nSect = bmExt.bmPages[i];
                if (!isSectNumValid(vol, nSect))
                {
                    if (adfEnv != null && adfEnv.wFct != null)
                    {
                        adfEnv.wFct.call("adfReadBitmap : sector out of range");
                    }
                }
                vol.bitmapBlocks[j] = nSect;

                if (adfReadBitmapBlock(vol, nSect, vol.bitmapTable[j]) != AdfError.RC_OK)
                {
                    adfFreeBitmap(vol);
                    return AdfError.RC_ERROR;
                }
                i++;
                j++;
            }
            nSect = bmExt.nextBlock;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfIsBlockFree
     *
     */

    public static boolean adfIsBlockFree(Volume vol, int nSect)
    {
        int sectOfMap = nSect - 2;
        int block = sectOfMap / (127 * 32);
        int indexInMap = (sectOfMap / 32) % 127;

        return ((vol.bitmapTable[block].map[indexInMap] & bitMask[sectOfMap % 32]) != 0);
    }

    /*
     * adfSetBlockFree OK
     *
     */

    public static void adfSetBlockFree(Volume vol, int nSect)
    {
        long oldValue = 0;
        int sectOfMap = nSect - 2;
        int block = sectOfMap / (127 * 32);
        int indexInMap = (sectOfMap / 32) % 127;

        oldValue = vol.bitmapTable[block].map[indexInMap];
        vol.bitmapTable[block].map[indexInMap] = oldValue | bitMask[sectOfMap % 32];

        vol.bitmapBlocksChg[block] = true;
    }

    /*
     * adfSetBlockUsed
     *
     */

    public static void adfSetBlockUsed(Volume vol, int nSect)
    {
        long oldValue = 0;
        int sectOfMap = nSect - 2;
        int block = sectOfMap / (127 * 32);
        int indexInMap = (sectOfMap / 32) % 127;

        oldValue = vol.bitmapTable[block].map[indexInMap];

        vol.bitmapTable[block].map[indexInMap] = oldValue & (~bitMask[sectOfMap % 32]);
        vol.bitmapBlocksChg[block] = true;
    }

    /*
     * adfGet1FreeBlock
     *
     */

    public static int adfGet1FreeBlock(Volume vol)
    {
        int[] block = new int[1];
        if (!adfGetFreeBlocks(vol, 1, block))
        {
            return -1;
        } else
        {
            return block[0];
        }
    }

    /*
     * adfGetFreeBlocks
     *
     */

    public static boolean adfGetFreeBlocks(Volume vol, int nbSect, int[] sectList)
    {
        int i = 0;
        int j = 0;
        boolean diskFull = false;
        int block = vol.rootBlock;

        i = 0;
        diskFull = false;
        while (i < nbSect && !diskFull)
        {
            if (adfIsBlockFree(vol, block))
            {
                sectList[i] = block;
                i++;
            }
            if ((block + vol.firstBlock) == vol.lastBlock)
            {
                block = 2;
            } else if (block == vol.rootBlock - 1)
            {
                diskFull = true;
            } else
            {
                block++;
            }
        }

        if (!diskFull)
        {
            for (j = 0; j < nbSect; j++)
            {
                adfSetBlockUsed(vol, sectList[j]);
            }
        }

        return (i == nbSect);
    }

    /*
     * adfCreateBitmap
     *
     * create bitmap structure in vol
     */

    public static AdfError adfCreateBitmap(Volume vol)
    {
        int nBlock = 0;
        int mapSize = 0;
        int i = 0;
        int j = 0;

        nBlock = vol.lastBlock - vol.firstBlock + 1 - 2;

        mapSize = nBlock / (127 * 32);
        if ((nBlock % (127 * 32)) != 0)
        {
            mapSize++;
        }
        vol.bitmapSize = mapSize;

        vol.bitmapTable = new BBitmapBlock[mapSize];
        if (vol.bitmapTable == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateBitmap : malloc, vol->bitmapTable");
            }
            return AdfError.RC_MALLOC;
        }

        vol.bitmapBlocksChg = new boolean[mapSize];
        if (vol.bitmapBlocksChg == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateBitmap : malloc, vol->bitmapBlocksChg");
            }
            return AdfError.RC_MALLOC;
        }

        vol.bitmapBlocks = new int[mapSize];
        if (vol.bitmapBlocks == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateBitmap : malloc, vol->bitmapBlocks");
            }
            return AdfError.RC_MALLOC;
        }

        for (i = 0; i < mapSize; i++)
        {
            vol.bitmapTable[i] = new BBitmapBlock();
            if (vol.bitmapTable[i] == null)
            {
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfCreateBitmap : malloc");
                }
                return AdfError.RC_MALLOC;
            }
        }

        for (i = vol.firstBlock + 2; i <= (vol.lastBlock - vol.firstBlock); i++)
        {
            adfSetBlockFree(vol, i);
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteNewBitmap
     *
     * write ext blocks and bitmap
     *
     * uses vol->bitmapSize,
     */

    public static AdfError adfWriteNewBitmap(Volume vol)
    {
        BBitmapExtBlock bitme = new BBitmapExtBlock();
        int[] bitExtBlock = null;
        int n = 0;
        int i = 0;
        int k = 0;
        int nExtBlock = 0;
        int nBlock = 0;
        int[] sectList = null;
        BRootBlock root = new BRootBlock();

        sectList = new int[vol.bitmapSize];
        if (sectList == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateBitmap : sectList");
            }
            return AdfError.RC_MALLOC;
        }

        if (!adfGetFreeBlocks(vol, vol.bitmapSize, sectList))
        {
            return AdfError.RC_ERROR;
        }

        if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }
        nBlock = 0;
        n = AdfConstants.min(vol.bitmapSize, AdfConstants.BM_SIZE);
        for (i = 0; i < n; i++)
        {
            root.bmPages[i] = vol.bitmapBlocks[i] = sectList[i];
        }
        nBlock = n;

        /* for devices with more than 25*127 blocks == hards disks */
        if (vol.bitmapSize > AdfConstants.BM_SIZE)
        {

            nExtBlock = (vol.bitmapSize - AdfConstants.BM_SIZE) / 127;
            if ((vol.bitmapSize - AdfConstants.BM_SIZE) % 127 != 0)
            {
                nExtBlock++;
            }

            bitExtBlock = new int[nExtBlock];
            if (bitExtBlock == null)
            {
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfWriteNewBitmap : malloc failed");
                }
                return AdfError.RC_MALLOC;
            }

            if (!adfGetFreeBlocks(vol, nExtBlock, bitExtBlock))
            {
                return AdfError.RC_MALLOC;
            }

            k = 0;
            root.bmExt = bitExtBlock[k];
            while (nBlock < vol.bitmapSize)
            {
                i = 0;
                while (i < 127 && nBlock < vol.bitmapSize)
                {
                    bitme.bmPages[i] = vol.bitmapBlocks[nBlock] = sectList[i];
                    i++;
                    nBlock++;
                }
                if (k + 1 < nExtBlock)
                {
                    bitme.nextBlock = bitExtBlock[k + 1];
                } else
                {
                    bitme.nextBlock = 0;
                }
                if (adfWriteBitmapExtBlock(vol, bitExtBlock[k], bitme) != AdfError.RC_OK)
                {
                    return AdfError.RC_ERROR;
                }
                k++;
            }

        }

        if (AdfRaw.adfWriteRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfReadBitmapBlock
     *
     * ENDIAN DEPENDENT
     */

    public static AdfError adfReadBitmapBlock(Volume vol, int nSect, BBitmapBlock bitm)
    {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BBitmapBlock tmp = BBitmapBlock.read(bb, 0);
        bitm.checkSum = tmp.checkSum;
        System.arraycopy(tmp.map, 0, bitm.map, 0, tmp.map.length);

        if (bitm.checkSum != AdfRaw.adfNormalSum(buf, 0, AdfConstants.LOGICAL_BLOCK_SIZE))
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfReadBitmapBlock : invalid checksum");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteBitmapBlock
     *
     * OK
     */

    public static AdfError adfWriteBitmapBlock(Volume vol, int nSect, BBitmapBlock bitm)
    {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        bitm.write(bb, 0);

        newSum = AdfRaw.adfNormalSum(buf, 0, AdfConstants.LOGICAL_BLOCK_SIZE);
        swLong(buf, 0, newSum);

        if (AdfDisk.adfWriteBlock(vol, nSect, buf) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfReadBitmapExtBlock
     *
     * ENDIAN DEPENDENT
     */

    public static AdfError adfReadBitmapExtBlock(Volume vol, int nSect, BBitmapExtBlock bitme)
    {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BBitmapExtBlock tmp = BBitmapExtBlock.read(bb, 0);
        System.arraycopy(tmp.bmPages, 0, bitme.bmPages, 0, tmp.bmPages.length);
        bitme.nextBlock = tmp.nextBlock;

        return AdfError.RC_OK;
    }

    /*
     * adfWriteBitmapExtBlock
     *
     */

    public static AdfError adfWriteBitmapExtBlock(Volume vol, int nSect, BBitmapExtBlock bitme)
    {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        bitme.write(bb, 0);

        if (AdfDisk.adfWriteBlock(vol, nSect, buf) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfFreeBitmap
     *
     */

    public static void adfFreeBitmap(Volume vol)
    {
        int i = 0;

        // bitmapTable entries will be GC'd
        vol.bitmapSize = 0;

        vol.bitmapTable = null;

        vol.bitmapBlocks = null;

        vol.bitmapBlocksChg = null;
    }

    // ------------------------------------------------------------------
    // Helpers
    // ------------------------------------------------------------------

    private static void swLong(byte[] buf, int off, long val)
    {
        buf[off] = (byte) ((val >> 24) & 0xFF);
        buf[off + 1] = (byte) ((val >> 16) & 0xFF);
        buf[off + 2] = (byte) ((val >> 8) & 0xFF);
        buf[off + 3] = (byte) (val & 0xFF);
    }

    private static boolean isSectNumValid(Volume vol, int nSect)
    {
        return 0 <= nSect && nSect <= (vol.lastBlock - vol.firstBlock);
    }

    private static DateTime adfGiveCurrentTime()
    {
        java.util.Calendar cal = java.util.Calendar.getInstance();
        DateTime dt = new DateTime();
        dt.year = cal.get(java.util.Calendar.YEAR) - 1900;
        dt.mon = cal.get(java.util.Calendar.MONTH) + 1;
        dt.day = cal.get(java.util.Calendar.DAY_OF_MONTH);
        dt.hour = cal.get(java.util.Calendar.HOUR_OF_DAY);
        dt.min = cal.get(java.util.Calendar.MINUTE);
        dt.sec = cal.get(java.util.Calendar.SECOND);
        return dt;
    }

    private static void adfTime2AmigaTime(DateTime dt, int[] day, int[] min, int[] ticks)
    {
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        min[0] = dt.hour * 60 + dt.min;
        ticks[0] = dt.sec * 50;

        day[0] = dt.day - 1;

        if (dt.mon > 1)
        {
            int mon = dt.mon - 1;
            if (mon > 2 && adfIsLeap(dt.year))
            {
                jm[1] = 29;
            }
            while (mon > 0)
            {
                day[0] = day[0] + jm[mon - 1];
                mon--;
            }
        }

        if (dt.year > 78)
        {
            int year = dt.year - 1;
            while (year >= 78)
            {
                if (adfIsLeap(year))
                {
                    day[0] = day[0] + 366;
                } else
                {
                    day[0] = day[0] + 365;
                }
                year--;
            }
        }
    }

    private static boolean adfIsLeap(int y)
    {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
    }
}
