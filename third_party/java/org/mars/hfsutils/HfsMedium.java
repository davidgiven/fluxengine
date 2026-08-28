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
 * $Id: medium.c,v 1.4 1998/11/02 22:09:04 rob Exp $
 */

package org.mars.hfsutils;

import java.util.Arrays;

import static org.mars.hfsutils.HfsConstants.*;

public final class HfsMedium
{
    private HfsMedium()
    {
    }

    /* Driver Descriptor Record Routines ======================================= */

    /*
     * NAME:	medium->zeroddr()
     * DESCRIPTION:	write a new/empty driver descriptor record
     */
    public static int m_zeroddr(HfsVol vol)
    {
        Block0 ddr = new Block0();

        /* ASSERT(vol->pnum == 0 && vol->vlen != 0); */

        ddr.sbSig       = (short) HFS_DDR_SIGWORD;
        ddr.sbBlkSize   = (short) HFS_BLOCKSZ;
        ddr.sbBlkCount  = (int) vol.vlen;

        ddr.sbDevType   = 0;
        ddr.sbDevId     = 0;
        ddr.sbData      = 0;

        ddr.sbDrvrCount = 0;

        ddr.ddBlock     = 0;
        ddr.ddSize      = 0;
        ddr.ddType      = 0;

        for (int i = 0; i < 243; ++i)
            ddr.ddPad[i] = 0;

        return HfsLow.l_putddr(vol, ddr);
    }

    /* Partition Map Routines ================================================== */

    /*
     * NAME:	medium->zeropm()
     * DESCRIPTION:	write new/empty partition map
     */
    public static int m_zeropm(HfsVol vol, int maxparts)
    {
        Partition map = new Partition();

        /* ASSERT(vol->pnum == 0 && vol->vlen != 0); */

        if (maxparts < 2)
            return fail(HfsException.EINVAL, "must allow at least 2 partitions");

        /* first entry: partition map itself */

        map.pmSig         = (short) HFS_PM_SIGWORD;
        map.pmSigPad      = 0;
        map.pmMapBlkCnt   = 2;

        map.pmPyPartStart = 1;
        map.pmPartBlkCnt  = maxparts;

        Arrays.fill(map.pmPartName, (char) 0);
        copyString(map.pmPartName, "Apple");
        Arrays.fill(map.pmParType, (char) 0);
        copyString(map.pmParType, "Apple_partition_map");

        map.pmLgDataStart = 0;
        map.pmDataCnt     = map.pmPartBlkCnt;

        map.pmPartStatus  = 0;

        map.pmLgBootStart = 0;
        map.pmBootSize    = 0;
        map.pmBootAddr    = 0;
        map.pmBootAddr2   = 0;
        map.pmBootEntry   = 0;
        map.pmBootEntry2  = 0;
        map.pmBootCksum   = 0;

        Arrays.fill(map.pmProcessor, (char) 0);

        for (int i = 0; i < 188; ++i)
            map.pmPad[i] = 0;

        if (HfsLow.l_putpmentry(vol, map, 1) == -1)
            return -1;

        /* second entry: rest of medium */

        map.pmPyPartStart = 1 + maxparts;
        map.pmPartBlkCnt  = (int) vol.vlen - 1 - maxparts;

        Arrays.fill(map.pmPartName, (char) 0);
        copyString(map.pmPartName, "Extra");
        Arrays.fill(map.pmParType, (char) 0);
        copyString(map.pmParType, "Apple_Free");

        map.pmDataCnt = map.pmPartBlkCnt;

        if (HfsLow.l_putpmentry(vol, map, 2) == -1)
            return -1;

        /* zero rest of partition map's partition */

        if (maxparts > 2)
        {
            byte[] b = new byte[HFS_BLOCKSZ];

            for (int i = 3; i <= maxparts; ++i)
            {
                if (HfsLow.b_writepb(vol, i, b, 1) == -1)
                    return -1;
            }
        }

        return 0;
    }

    /*
     * NAME:	medium->findpmentry()
     * DESCRIPTION:	locate a partition map entry
     */
    public static int m_findpmentry(HfsVol vol, String type,
                                    Partition map, long[] start)
    {
        long bnum;
        int found = 0;

        if (start != null && start[0] > 0)
        {
            bnum = start[0];

            if (bnum++ >= (long) map.pmMapBlkCnt)
            {
                fail(HfsException.EINVAL, "partition not found");
                return found;
            }
        }
        else
            bnum = 1;

        while (true)
        {
            if (HfsLow.l_getpmentry(vol, map, bnum) == -1)
            {
                found = -1;
                return found;
            }

            if (map.pmSig != (short) HFS_PM_SIGWORD)
            {
                found = -1;

                if (map.pmSig == (short) HFS_PM_SIGWORD_OLD)
                {
                    fail(HfsException.EINVAL, "old partition map format not supported");
                    return found;
                }
                else
                {
                    fail(HfsException.EINVAL, "invalid partition map");
                    return found;
                }
            }

            if (comparePmType(map, type))
            {
                found = 1;
                break;
            }

            if (bnum++ >= (long) map.pmMapBlkCnt)
            {
                fail(HfsException.EINVAL, "partition not found");
                return found;
            }
        }

        if (start != null)
            start[0] = bnum;

        return found;
    }

    /*
     * NAME:	medium->mkpart()
     * DESCRIPTION:	create a new partition from available free space
     */
    public static int m_mkpart(HfsVol vol,
                               String name, String type, long len)
    {
        Partition map = new Partition();
        int nparts;
        int maxparts;
        long bnum;
        long start;
        long remain;
        int found;

        if (name.length() > 32 ||
            type.length() > 32)
            return fail(HfsException.EINVAL, "partition name/type can each be at most 32 chars");

        if (len == 0)
            return fail(HfsException.EINVAL, "partition length must be > 0");

        found = m_findpmentry(vol, "Apple_partition_map", map, null);
        if (found == -1)
            return -1;

        if (found == 0)
            return fail(HfsException.EIO, "cannot find partition map's partition");

        nparts   = map.pmMapBlkCnt;
        maxparts = map.pmPartBlkCnt;

        bnum = 0;
        do
        {
            found = m_findpmentry(vol, "Apple_Free", map, new long[]{ bnum });
            if (found == -1)
                return -1;

            if (found == 0)
                return fail(HfsException.ENOSPC, "no available partitions");
        }
        while (len > (long) map.pmPartBlkCnt);

        start  = (long) map.pmPyPartStart + len;
        remain = (long) map.pmPartBlkCnt  - len;

        if (remain != 0 && nparts >= maxparts)
            return fail(HfsException.EINVAL, "must allocate all blocks in free space");

        map.pmPartBlkCnt = (int) len;

        Arrays.fill(map.pmPartName, (char) 0);
        copyString(map.pmPartName, name);
        Arrays.fill(map.pmParType, (char) 0);
        copyString(map.pmParType, type);

        map.pmLgDataStart = 0;
        map.pmDataCnt     = (int) len;

        map.pmPartStatus  = 0;

        if (HfsLow.l_putpmentry(vol, map, bnum) == -1)
            return -1;

        if (remain != 0)
        {
            map.pmPyPartStart = (int) start;
            map.pmPartBlkCnt  = (int) remain;

            Arrays.fill(map.pmPartName, (char) 0);
            copyString(map.pmPartName, "Extra");
            Arrays.fill(map.pmParType, (char) 0);
            copyString(map.pmParType, "Apple_Free");

            map.pmDataCnt = (int) remain;

            if (HfsLow.l_putpmentry(vol, map, ++nparts) == -1)
                return -1;

            for (bnum = 1; bnum <= nparts; ++bnum)
            {
                if (HfsLow.l_getpmentry(vol, map, bnum) == -1)
                    return -1;

                map.pmMapBlkCnt = nparts;

                if (HfsLow.l_putpmentry(vol, map, bnum) == -1)
                    return -1;
            }
        }

        return 0;
    }

    /* Boot Blocks Routines ==================================================== */

    /*
     * NAME:	medium->zerobb()
     * DESCRIPTION:	write new/empty volume boot blocks
     */
    public static int m_zerobb(HfsVol vol)
    {
        byte[] b = new byte[HFS_BLOCKSZ];

        if (HfsLow.b_writelb(vol, 0, b) == -1 ||
            HfsLow.b_writelb(vol, 1, b) == -1)
            return -1;

        return 0;
    }

    /* Helper methods ========================================================== */

    private static int fail(int errno, String msg)
    {
        Hfs.hfsError = msg;
        Hfs.hfsErrno = errno;
        return -1;
    }

    private static void copyString(char[] dest, String src)
    {
        int len = Math.min(src.length(), dest.length);
        for (int i = 0; i < len; i++)
            dest[i] = src.charAt(i);
    }

    private static boolean comparePmType(Partition map, String type)
    {
        int len = Math.min(type.length(), map.pmParType.length);
        for (int i = 0; i < len; i++)
        {
            if (map.pmParType[i] != type.charAt(i))
                return false;
        }

        if (len < map.pmParType.length)
            return map.pmParType[len] == 0;

        return true;
    }
}
