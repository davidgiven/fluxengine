/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: low.c,v 1.8 1998/11/02 22:09:03 rob Exp $
 */

package org.mars.hfsutils;

import static org.mars.hfsutils.HfsConstants.*;

public final class HfsLow
{
    private HfsLow()
    {
    }

    /* Block I/O ============================================================ */

    /*
     * NAME:	low->readpb()
     * DESCRIPTION:	read blocks from the physical medium (bypassing cache)
     */
    public static int b_readpb(HfsVol vol, long bnum, byte[] bp, int blen)
    {
        long nblocks;

        nblocks = vol.priv.seek(bnum);
        if (nblocks == -1)
            return -1;

        if (nblocks != bnum)
            return -1;

        nblocks = vol.priv.read(bp, blen);
        if (nblocks == -1)
            return -1;

        if (nblocks != blen)
            return -1;

        return 0;
    }

    /*
     * NAME:	low->writepb()
     * DESCRIPTION:	write blocks to the physical medium (bypassing cache)
     */
    public static int b_writepb(HfsVol vol, long bnum, byte[] bp, int blen)
    {
        long nblocks;

        nblocks = vol.priv.seek(bnum);
        if (nblocks == -1)
            return -1;

        if (nblocks != bnum)
            return -1;

        nblocks = vol.priv.write(bp, blen);
        if (nblocks == -1)
            return -1;

        if (nblocks != blen)
            return -1;

        return 0;
    }

    /*
     * NAME:	low->readlb()
     * DESCRIPTION:	read a logical block from a volume
     */
    public static int b_readlb(HfsVol vol, long bnum, byte[] bp)
    {
        if (vol.vlen > 0 && bnum >= vol.vlen)
            return -1;

        if (b_readpb(vol, vol.vstart + bnum, bp, 1) == -1)
            return -1;

        return 0;
    }

    /*
     * NAME:	low->writelb()
     * DESCRIPTION:	write a logical block to a volume
     */
    public static int b_writelb(HfsVol vol, long bnum, byte[] bp)
    {
        if (vol.vlen > 0 && bnum >= vol.vlen)
            return -1;

        if (b_writepb(vol, vol.vstart + bnum, bp, 1) == -1)
            return -1;

        return 0;
    }

    /* Driver Descriptor Record ============================================ */

    /*
     * NAME:	low->getddr()
     * DESCRIPTION:	read a driver descriptor record
     */
    public static int l_getddr(HfsVol vol, Block0 ddr)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        if (b_readpb(vol, 0, b, 1) == -1)
            return -1;

        ddr.sbSig = HfsData.d_fetchsw(b, cursor);
        ddr.sbBlkSize = HfsData.d_fetchsw(b, cursor);
        ddr.sbBlkCount = (int) HfsData.d_fetchsl(b, cursor);
        ddr.sbDevType = HfsData.d_fetchsw(b, cursor);
        ddr.sbDevId = HfsData.d_fetchsw(b, cursor);
        ddr.sbData = HfsData.d_fetchsl(b, cursor);
        ddr.sbDrvrCount = HfsData.d_fetchsw(b, cursor);
        ddr.ddBlock = HfsData.d_fetchsl(b, cursor);
        ddr.ddSize = HfsData.d_fetchsw(b, cursor);
        ddr.ddType = HfsData.d_fetchsw(b, cursor);

        for (i = 0; i < 243; ++i)
            ddr.ddPad[i] = HfsData.d_fetchsw(b, cursor);

        // #ifdef DEBUG
        assert cursor[0] == HFS_BLOCKSZ;
        // #endif

        return 0;
    }

    /*
     * NAME:	low->putddr()
     * DESCRIPTION:	write a driver descriptor record
     */
    public static int l_putddr(HfsVol vol, Block0 ddr)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        HfsData.d_storesw(b, cursor, ddr.sbSig);
        HfsData.d_storesw(b, cursor, ddr.sbBlkSize);
        HfsData.d_storesl(b, cursor, ddr.sbBlkCount);
        HfsData.d_storesw(b, cursor, ddr.sbDevType);
        HfsData.d_storesw(b, cursor, ddr.sbDevId);
        HfsData.d_storesl(b, cursor, ddr.sbData);
        HfsData.d_storesw(b, cursor, ddr.sbDrvrCount);
        HfsData.d_storesl(b, cursor, ddr.ddBlock);
        HfsData.d_storesw(b, cursor, ddr.ddSize);
        HfsData.d_storesw(b, cursor, ddr.ddType);

        for (i = 0; i < 243; ++i)
            HfsData.d_storesw(b, cursor, ddr.ddPad[i]);

        // #ifdef DEBUG
        assert cursor[0] == HFS_BLOCKSZ;
        // #endif

        if (b_writepb(vol, 0, b, 1) == -1)
            return -1;

        return 0;
    }

    /* Partition Map ======================================================== */

    /*
     * NAME:	low->getpmentry()
     * DESCRIPTION:	read a partition map entry
     */
    public static int l_getpmentry(HfsVol vol, Partition map, long bnum)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        if (b_readpb(vol, bnum, b, 1) == -1)
            return -1;

        map.pmSig = HfsData.d_fetchsw(b, cursor);
        map.pmSigPad = HfsData.d_fetchsw(b, cursor);
        map.pmMapBlkCnt = (int) HfsData.d_fetchsl(b, cursor);
        map.pmPyPartStart = (int) HfsData.d_fetchsl(b, cursor);
        map.pmPartBlkCnt = (int) HfsData.d_fetchsl(b, cursor);

        for (i = 0; i < 32; i++)
            map.pmPartName[i] = (char) (b[cursor[0] + i] & 0xff);
        map.pmPartName[32] = 0;
        cursor[0] += 32;

        for (i = 0; i < 32; i++)
            map.pmParType[i] = (char) (b[cursor[0] + i] & 0xff);
        map.pmParType[32] = 0;
        cursor[0] += 32;

        map.pmLgDataStart = (int) HfsData.d_fetchsl(b, cursor);
        map.pmDataCnt = (int) HfsData.d_fetchsl(b, cursor);
        map.pmPartStatus = (int) HfsData.d_fetchsl(b, cursor);
        map.pmLgBootStart = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootSize = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootAddr = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootAddr2 = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootEntry = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootEntry2 = (int) HfsData.d_fetchsl(b, cursor);
        map.pmBootCksum = (int) HfsData.d_fetchsl(b, cursor);

        for (i = 0; i < 16; i++)
            map.pmProcessor[i] = (char) (b[cursor[0] + i] & 0xff);
        map.pmProcessor[16] = 0;
        cursor[0] += 16;

        for (i = 0; i < 188; ++i)
            map.pmPad[i] = HfsData.d_fetchsw(b, cursor);

        // #ifdef DEBUG
        assert cursor[0] == HFS_BLOCKSZ;
        // #endif

        return 0;
    }

    /*
     * NAME:	low->putpmentry()
     * DESCRIPTION:	write a partition map entry
     */
    public static int l_putpmentry(HfsVol vol, Partition map, long bnum)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        HfsData.d_storesw(b, cursor, map.pmSig);
        HfsData.d_storesw(b, cursor, map.pmSigPad);
        HfsData.d_storesl(b, cursor, map.pmMapBlkCnt);
        HfsData.d_storesl(b, cursor, map.pmPyPartStart);
        HfsData.d_storesl(b, cursor, map.pmPartBlkCnt);

        for (i = 0; i < 32; i++)
            b[cursor[0] + i] = 0;
        for (i = 0; i < 32 && map.pmPartName[i] != 0; i++)
            b[cursor[0] + i] = (byte) map.pmPartName[i];
        cursor[0] += 32;

        for (i = 0; i < 32; i++)
            b[cursor[0] + i] = 0;
        for (i = 0; i < 32 && map.pmParType[i] != 0; i++)
            b[cursor[0] + i] = (byte) map.pmParType[i];
        cursor[0] += 32;

        HfsData.d_storesl(b, cursor, map.pmLgDataStart);
        HfsData.d_storesl(b, cursor, map.pmDataCnt);
        HfsData.d_storesl(b, cursor, map.pmPartStatus);
        HfsData.d_storesl(b, cursor, map.pmLgBootStart);
        HfsData.d_storesl(b, cursor, map.pmBootSize);
        HfsData.d_storesl(b, cursor, map.pmBootAddr);
        HfsData.d_storesl(b, cursor, map.pmBootAddr2);
        HfsData.d_storesl(b, cursor, map.pmBootEntry);
        HfsData.d_storesl(b, cursor, map.pmBootEntry2);
        HfsData.d_storesl(b, cursor, map.pmBootCksum);

        for (i = 0; i < 16; i++)
            b[cursor[0] + i] = 0;
        for (i = 0; i < 16 && map.pmProcessor[i] != 0; i++)
            b[cursor[0] + i] = (byte) map.pmProcessor[i];
        cursor[0] += 16;

        for (i = 0; i < 188; ++i)
            HfsData.d_storesw(b, cursor, map.pmPad[i]);

        // #ifdef DEBUG
        assert cursor[0] == HFS_BLOCKSZ;
        // #endif

        if (b_writepb(vol, bnum, b, 1) == -1)
            return -1;

        return 0;
    }

    /* Boot Blocks ========================================================== */

    /*
     * NAME:	low->getbb()
     * DESCRIPTION:	read a volume's boot blocks
     */
    public static int l_getbb(HfsVol vol, BootBlkHdr bb, byte[] bootcode)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};

        if (b_readlb(vol, 0, b) == -1)
            return -1;

        bb.bbID = HfsData.d_fetchsw(b, cursor);
        bb.bbEntry = HfsData.d_fetchsl(b, cursor);
        bb.bbVersion = HfsData.d_fetchsw(b, cursor);
        bb.bbPageFlags = HfsData.d_fetchsw(b, cursor);

        HfsData.d_fetchstr(b, cursor, bb.bbSysName,    16);
        HfsData.d_fetchstr(b, cursor, bb.bbShellName,  16);
        HfsData.d_fetchstr(b, cursor, bb.bbDbg1Name,   16);
        HfsData.d_fetchstr(b, cursor, bb.bbDbg2Name,   16);
        HfsData.d_fetchstr(b, cursor, bb.bbScreenName, 16);
        HfsData.d_fetchstr(b, cursor, bb.bbHelloName,  16);
        HfsData.d_fetchstr(b, cursor, bb.bbScrapName,  16);

        bb.bbCntFCBs = HfsData.d_fetchsw(b, cursor);
        bb.bbCntEvts = HfsData.d_fetchsw(b, cursor);
        bb.bb128KSHeap = HfsData.d_fetchsl(b, cursor);
        bb.bb256KSHeap = HfsData.d_fetchsl(b, cursor);
        bb.bbSysHeapSize = HfsData.d_fetchsl(b, cursor);
        bb.filler = HfsData.d_fetchsw(b, cursor);
        bb.bbSysHeapExtra = HfsData.d_fetchsl(b, cursor);
        bb.bbSysHeapFract = HfsData.d_fetchsl(b, cursor);

        // #ifdef DEBUG
        assert cursor[0] == 148;
        // #endif

        if (bootcode != null)
        {
            System.arraycopy(b, cursor[0], bootcode, 0, HFS_BOOTCODE1LEN);

            if (b_readlb(vol, 1, b) == -1)
                return -1;

            System.arraycopy(b, 0, bootcode, HFS_BOOTCODE1LEN, HFS_BOOTCODE2LEN);
        }

        return 0;
    }

    /*
     * NAME:	low->putbb()
     * DESCRIPTION:	write a volume's boot blocks
     */
    public static int l_putbb(HfsVol vol, BootBlkHdr bb, byte[] bootcode)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};

        HfsData.d_storesw(b, cursor, bb.bbID);
        HfsData.d_storesl(b, cursor, bb.bbEntry);
        HfsData.d_storesw(b, cursor, bb.bbVersion);
        HfsData.d_storesw(b, cursor, bb.bbPageFlags);

        HfsData.d_storestr(b, cursor, bb.bbSysName,    16);
        HfsData.d_storestr(b, cursor, bb.bbShellName,  16);
        HfsData.d_storestr(b, cursor, bb.bbDbg1Name,   16);
        HfsData.d_storestr(b, cursor, bb.bbDbg2Name,   16);
        HfsData.d_storestr(b, cursor, bb.bbScreenName, 16);
        HfsData.d_storestr(b, cursor, bb.bbHelloName,  16);
        HfsData.d_storestr(b, cursor, bb.bbScrapName,  16);

        HfsData.d_storesw(b, cursor, bb.bbCntFCBs);
        HfsData.d_storesw(b, cursor, bb.bbCntEvts);
        HfsData.d_storesl(b, cursor, bb.bb128KSHeap);
        HfsData.d_storesl(b, cursor, bb.bb256KSHeap);
        HfsData.d_storesl(b, cursor, bb.bbSysHeapSize);
        HfsData.d_storesw(b, cursor, bb.filler);
        HfsData.d_storesl(b, cursor, bb.bbSysHeapExtra);
        HfsData.d_storesl(b, cursor, bb.bbSysHeapFract);

        // #ifdef DEBUG
        assert cursor[0] == 148;
        // #endif

        if (bootcode != null)
            System.arraycopy(bootcode, 0, b, cursor[0], HFS_BOOTCODE1LEN);
        else
        {
            for (int j = cursor[0]; j < cursor[0] + HFS_BOOTCODE1LEN; j++)
                b[j] = 0;
        }

        if (b_writelb(vol, 0, b) == -1)
            return -1;

        if (bootcode != null)
            System.arraycopy(bootcode, HFS_BOOTCODE1LEN, b, 0, HFS_BOOTCODE2LEN);
        else
        {
            for (int j = 0; j < HFS_BOOTCODE2LEN; j++)
                b[j] = 0;
        }

        if (b_writelb(vol, 1, b) == -1)
            return -1;

        return 0;
    }

    /* Master Directory Block =============================================== */

    /*
     * NAME:	low->getmdb()
     * DESCRIPTION:	read a master directory block
     */
    public static int l_getmdb(HfsVol vol, Mdb mdb, boolean backup)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        if (b_readlb(vol, backup ? vol.vlen - 2 : 2, b) == -1)
            return -1;

        mdb.drSigWord = HfsData.d_fetchsw(b, cursor);
        mdb.drCrDate = HfsData.d_fetchsl(b, cursor);
        mdb.drLsMod = HfsData.d_fetchsl(b, cursor);
        mdb.drAtrb = HfsData.d_fetchsw(b, cursor);
        mdb.drNmFls = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drVBMSt = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drAllocPtr = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drNmAlBlks = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drAlBlkSiz = HfsData.d_fetchul(b, cursor);
        mdb.drClpSiz = HfsData.d_fetchul(b, cursor);
        mdb.drAlBlSt = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drNxtCNID = HfsData.d_fetchsl(b, cursor);
        mdb.drFreeBks = (short) HfsData.d_fetchuw(b, cursor);

        HfsData.d_fetchstr(b, cursor, mdb.drVN, 28);

        // #ifdef DEBUG
        assert cursor[0] == 64;
        // #endif

        mdb.drVolBkUp = HfsData.d_fetchsl(b, cursor);
        mdb.drVSeqNum = HfsData.d_fetchsw(b, cursor);
        mdb.drWrCnt = HfsData.d_fetchul(b, cursor);
        mdb.drXTClpSiz = HfsData.d_fetchul(b, cursor);
        mdb.drCTClpSiz = HfsData.d_fetchul(b, cursor);
        mdb.drNmRtDirs = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drFilCnt = HfsData.d_fetchul(b, cursor);
        mdb.drDirCnt = HfsData.d_fetchul(b, cursor);

        for (i = 0; i < 8; ++i)
            mdb.drFndrInfo[i] = HfsData.d_fetchsl(b, cursor);

        // #ifdef DEBUG
        assert cursor[0] == 124;
        // #endif

        mdb.drEmbedSigWord = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drEmbedExtent.xdrStABN = (short) HfsData.d_fetchuw(b, cursor);
        mdb.drEmbedExtent.xdrNumABlks = (short) HfsData.d_fetchuw(b, cursor);

        mdb.drXTFlSize = HfsData.d_fetchul(b, cursor);

        for (i = 0; i < 3; ++i)
        {
            mdb.drXTExtRec.data[i].xdrStABN = (short) HfsData.d_fetchuw(b, cursor);
            mdb.drXTExtRec.data[i].xdrNumABlks = (short) HfsData.d_fetchuw(b, cursor);
        }

        // #ifdef DEBUG
        assert cursor[0] == 146;
        // #endif

        mdb.drCTFlSize = HfsData.d_fetchul(b, cursor);

        for (i = 0; i < 3; ++i)
        {
            mdb.drCTExtRec.data[i].xdrStABN = (short) HfsData.d_fetchuw(b, cursor);
            mdb.drCTExtRec.data[i].xdrNumABlks = (short) HfsData.d_fetchuw(b, cursor);
        }

        // #ifdef DEBUG
        assert cursor[0] == 162;
        // #endif

        return 0;
    }

    /*
     * NAME:	low->putmdb()
     * DESCRIPTION:	write master directory block(s)
     */
    public static int l_putmdb(HfsVol vol, Mdb mdb, boolean backup)
    {
        byte[] b = new byte[HFS_BLOCKSZ];
        int[] cursor = new int[]{0};
        int i;

        HfsData.d_storesw(b, cursor, mdb.drSigWord);
        HfsData.d_storesl(b, cursor, mdb.drCrDate);
        HfsData.d_storesl(b, cursor, mdb.drLsMod);
        HfsData.d_storesw(b, cursor, mdb.drAtrb);
        HfsData.d_storeuw(b, cursor, mdb.drNmFls);
        HfsData.d_storeuw(b, cursor, mdb.drVBMSt);
        HfsData.d_storeuw(b, cursor, mdb.drAllocPtr);
        HfsData.d_storeuw(b, cursor, mdb.drNmAlBlks);
        HfsData.d_storeul(b, cursor, mdb.drAlBlkSiz);
        HfsData.d_storeul(b, cursor, mdb.drClpSiz);
        HfsData.d_storeuw(b, cursor, mdb.drAlBlSt);
        HfsData.d_storesl(b, cursor, mdb.drNxtCNID);
        HfsData.d_storeuw(b, cursor, mdb.drFreeBks);

        HfsData.d_storestr(b, cursor, mdb.drVN, 28);

        // #ifdef DEBUG
        assert cursor[0] == 64;
        // #endif

        HfsData.d_storesl(b, cursor, mdb.drVolBkUp);
        HfsData.d_storesw(b, cursor, mdb.drVSeqNum);
        HfsData.d_storeul(b, cursor, mdb.drWrCnt);
        HfsData.d_storeul(b, cursor, mdb.drXTClpSiz);
        HfsData.d_storeul(b, cursor, mdb.drCTClpSiz);
        HfsData.d_storeuw(b, cursor, mdb.drNmRtDirs);
        HfsData.d_storeul(b, cursor, mdb.drFilCnt);
        HfsData.d_storeul(b, cursor, mdb.drDirCnt);

        for (i = 0; i < 8; ++i)
            HfsData.d_storesl(b, cursor, mdb.drFndrInfo[i]);

        // #ifdef DEBUG
        assert cursor[0] == 124;
        // #endif

        HfsData.d_storeuw(b, cursor, mdb.drEmbedSigWord);
        HfsData.d_storeuw(b, cursor, mdb.drEmbedExtent.xdrStABN);
        HfsData.d_storeuw(b, cursor, mdb.drEmbedExtent.xdrNumABlks);

        HfsData.d_storeul(b, cursor, mdb.drXTFlSize);

        for (i = 0; i < 3; ++i)
        {
            HfsData.d_storeuw(b, cursor, mdb.drXTExtRec.data[i].xdrStABN);
            HfsData.d_storeuw(b, cursor, mdb.drXTExtRec.data[i].xdrNumABlks);
        }

        // #ifdef DEBUG
        assert cursor[0] == 146;
        // #endif

        HfsData.d_storeul(b, cursor, mdb.drCTFlSize);

        for (i = 0; i < 3; ++i)
        {
            HfsData.d_storeuw(b, cursor, mdb.drCTExtRec.data[i].xdrStABN);
            HfsData.d_storeuw(b, cursor, mdb.drCTExtRec.data[i].xdrNumABlks);
        }

        // #ifdef DEBUG
        assert cursor[0] == 162;
        // #endif

        for (i = cursor[0]; i < HFS_BLOCKSZ; i++)
            b[i] = 0;

        if (b_writelb(vol, 2, b) == -1 ||
            (backup && b_writelb(vol, vol.vlen - 2, b) == -1))
            return -1;

        return 0;
    }
}
