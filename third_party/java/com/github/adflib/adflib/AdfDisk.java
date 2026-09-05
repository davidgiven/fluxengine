/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_disk.c / adf_disk.h
 *
 *  $Id$
 *
 *  logical disk/volume code
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
import java.util.Arrays;

/**
 * Java port of {@code adf_disk.c} / {@code adf_disk.h}.
 *
 * <p>Logical disk / volume code. Keeps original C control flow and helper
 * naming. ByteBuffer I/O uses absolute {@code get(int)}/{@code put(int,byte)}
 * without touching {@code position}/{@code limit} and honours
 * {@code BIG_ENDIAN} for ADF data via {@link AdfEndian}.
 */
public final class AdfDisk
{

    private AdfDisk()
    {
    }

    /**
     * Global environment — mirrors {@code extern struct Env adfEnv}.
     */
    public static Env adfEnv = AdfRaw.adfEnv;

    public static final int[] bitMask = {0x1,
            0x2,
            0x4,
            0x8,
            0x10,
            0x20,
            0x40,
            0x80,
            0x100,
            0x200,
            0x400,
            0x800,
            0x1000,
            0x2000,
            0x4000,
            0x8000,
            0x10000,
            0x20000,
            0x40000,
            0x80000,
            0x100000,
            0x200000,
            0x400000,
            0x800000,
            0x1000000,
            0x2000000,
            0x4000000,
            0x8000000,
            0x10000000,
            0x20000000,
            0x40000000,
            0x80000000};

    public static AdfError adfInstallBootBlock(Volume vol, byte[] code)
    {
        int i = 0;
        BBootBlock boot = new BBootBlock();

        if (vol.dev.devType != AdfConstants.DEVTYPE_FLOPDD &&
                vol.dev.devType != AdfConstants.DEVTYPE_FLOPHD)
        {
            return AdfError.RC_ERROR;
        }

        if (AdfRaw.adfReadBootBlock(vol, boot) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        boot.rootBlock = 880;
        for (i = 0; i < 1024 - 12; i++)
        {
            boot.data[i] = code[i + 12];
        }

        if (AdfRaw.adfWriteBootBlock(vol, boot) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        vol.bootCode = true;

        return AdfError.RC_OK;
    }

    /*
     * isSectNumValid
     *
     */

    public static boolean isSectNumValid(Volume vol, int nSect)
    {
        return 0 <= nSect && nSect <= (vol.lastBlock - vol.firstBlock);
    }

    /*
     * adfVolumeInfo
     *
     */

    public static void adfVolumeInfo(Volume vol)
    {
        BRootBlock root = new BRootBlock();
        byte[] diskName = new byte[35];
        int days = 0;
        int month = 0;
        int year = 0;
        int[] yy = new int[1];
        int[] mm = new int[1];
        int[] dd = new int[1];

        if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return;
        }

        Arrays.fill(diskName, (byte) 0);
        System.arraycopy(root.diskName, 0, diskName, 0, root.nameLen & 0xFF);

        System.out.printf("Name : %-30s\n", vol.volName);
        System.out.printf("Type : ");
        switch (vol.dev.devType)
        {
            case AdfConstants.DEVTYPE_FLOPDD:
                System.out.printf("Floppy Double Density : 880 KBytes\n");
                break;
            case AdfConstants.DEVTYPE_FLOPHD:
                System.out.printf("Floppy High Density : 1760 KBytes\n");
                break;
            case AdfConstants.DEVTYPE_HARDDISK:
                System.out.printf(
                        "Hard Disk partition : %3.1f KBytes\n",
                        (vol.lastBlock - vol.firstBlock + 1) * 512.0 / 1024.0);
                break;
            case AdfConstants.DEVTYPE_HARDFILE:
                System.out.printf(
                        "HardFile : %3.1f KBytes\n",
                        (vol.lastBlock - vol.firstBlock + 1) * 512.0 / 1024.0);
                break;
            default:
                System.out.printf("Unknown devType!\n");
        }
        System.out.printf("Filesystem : ");
        System.out.printf("%s ", AdfConstants.isFFS(vol.dosType & 0xFF) ? "FFS" : "OFS");
        if (AdfConstants.isINTL(vol.dosType & 0xFF))
        {
            System.out.printf("INTL ");
        }
        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF))
        {
            System.out.printf("DIRCACHE ");
        }
        System.out.printf("\n");

        System.out.printf("Free blocks = %d\n", adfCountFreeBlocks(vol));
        if (vol.readOnly)
        {
            System.out.printf("Read only\n");
        } else
        {
            System.out.printf("Read/Write\n");
        }

        /* created */
        adfDays2Date(root.coDays, yy, mm, dd);
        year = yy[0];
        month = mm[0];
        days = dd[0];
        System.out.printf(
                "created %d/%02d/%02d %d:%02d:%02d\n",
                days,
                month,
                year,
                root.coMins / 60,
                root.coMins % 60,
                root.coTicks / 50);
        adfDays2Date(root.days, yy, mm, dd);
        year = yy[0];
        month = mm[0];
        days = dd[0];
        System.out.printf(
                "last access %d/%02d/%02d %d:%02d:%02d,   ",
                days,
                month,
                year,
                root.mins / 60,
                root.mins % 60,
                root.ticks / 50);
        adfDays2Date(root.cDays, yy, mm, dd);
        year = yy[0];
        month = mm[0];
        days = dd[0];
        System.out.printf(
                "%d/%02d/%02d %d:%02d:%02d\n",
                days,
                month,
                year,
                root.cMins / 60,
                root.cMins % 60,
                root.cTicks / 50);
    }

    /*
     * adfMount
     *
     *
     */

    public static Volume adfMount(Device dev, int nPart, boolean readOnly)
    {
        int nBlock = 0;
        BRootBlock root = new BRootBlock();
        BBootBlock boot = new BBootBlock();
        Volume vol = null;

        if (dev == null || nPart >= dev.nVol)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMount : invalid parameter(s)");
            }
            return null;
        }

        vol = dev.volList.get(nPart);
        vol.dev = dev;
        vol.mounted = true;

        if (AdfRaw.adfReadBootBlock(vol, boot) != AdfError.RC_OK)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfMount : BootBlock invalid");
            }
            return null;
        }

        vol.dosType = boot.dosType[3];
        if (AdfConstants.isFFS(vol.dosType & 0xFF))
        {
            vol.datablockSize = 512;
        } else
        {
            vol.datablockSize = 488;
        }

        if (dev.readOnly)
        {
            vol.readOnly = true;
        } else
        {
            vol.readOnly = readOnly;
        }

        if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfMount : RootBlock invalid");
            }
            return null;
        }

        nBlock = vol.lastBlock - vol.firstBlock + 1 - 2;

        adfReadBitmap(vol, nBlock, root);
        vol.curDirPtr = vol.rootBlock;

        return vol;
    }

    /*
     *
     * adfUnMount
     *
     * free bitmap structures
     * free current dir
     */

    public static void adfUnMount(Volume vol)
    {
        if (vol == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfUnMount : vol is null");
            }
            return;
        }

        adfFreeBitmap(vol);

        vol.mounted = false;
    }

    /*
     * adfCreateVol
     *
     *
     */

    public static Volume adfCreateVol(Device dev, int start, int len, String volName, int volType)
    {
        BBootBlock boot = new BBootBlock();
        BRootBlock root = new BRootBlock();
        int[] blkList = new int[2];
        Volume vol = null;
        int nlen = 0;

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(0);
        }

        vol = new Volume();
        if (vol == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateVol : malloc vol");
            }
            return null;
        }

        vol.dev = dev;
        vol.firstBlock = (dev.heads * dev.sectors) * start;
        vol.lastBlock = (vol.firstBlock + (dev.heads * dev.sectors) * len) - 1;
        vol.rootBlock = (vol.lastBlock - vol.firstBlock + 1) / 2;
        vol.curDirPtr = vol.rootBlock;

        vol.readOnly = dev.readOnly;

        vol.mounted = true;

        nlen = AdfConstants.min(AdfConstants.MAXNAMELEN, volName.length());
        vol.volName = volName.substring(0, nlen);

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(25);
        }

        boot.dosType[3] = (byte) volType;
        if (AdfRaw.adfWriteBootBlock(vol, boot) != AdfError.RC_OK)
        {
            return null;
        }

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(20);
        }

        if (adfCreateBitmap(vol) != AdfError.RC_OK)
        {
            return null;
        }

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(40);
        }

        if (AdfConstants.isDIRCACHE(volType))
        {
            adfGetFreeBlocks(vol, 2, blkList);
        } else
        {
            adfGetFreeBlocks(vol, 1, blkList);
        }

        root.nameLen = (byte) volName.length();
        if (root.nameLen > AdfConstants.MAXNAMELEN)
        {
            root.nameLen = (byte) AdfConstants.MAXNAMELEN;
        }
        byte[] nameBytes = volName.getBytes();
        for (int i = 0; i < (root.nameLen & 0xFF); i++)
        {
            root.diskName[i] = nameBytes[i];
        }
        DateTime dt = adfGiveCurrentTime();
        int[] day = new int[1];
        int[] min = new int[1];
        int[] ticks = new int[1];
        adfTime2AmigaTime(dt, day, min, ticks);
        root.coDays = day[0];
        root.coMins = min[0];
        root.coTicks = ticks[0];

        /* dircache block */
        if (AdfConstants.isDIRCACHE(volType))
        {
            root.extension = 0;
            root.secType = AdfConstants.ST_ROOT; /* needed by adfCreateEmptyCache() */
            adfCreateEmptyCache(vol, root, blkList[1]);
        }

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(60);
        }

        if (AdfRaw.adfWriteRootBlock(vol, blkList[0], root) != AdfError.RC_OK)
        {
            return null;
        }

        /* fills root->bmPages[] and writes filled bitmapExtBlocks */
        if (adfWriteNewBitmap(vol) != AdfError.RC_OK)
        {
            return null;
        }

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(80);
        }

        if (adfUpdateBitmap(vol) != AdfError.RC_OK)
        {
            return null;
        }

        if (adfEnv != null && adfEnv.useProgressBar && adfEnv.progressBar != null)
        {
            adfEnv.progressBar.progress(100);
        }

        /* will be managed by adfMount() later */
        adfFreeBitmap(vol);

        vol.mounted = false;

        return vol;
    }

    /*-----*/

    /*
     * adfReadBlock
     *
     * read logical block
     */

    public static AdfError adfReadBlock(Volume vol, int nSect, byte[] buf)
    {
        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        return adfReadBlock(vol, nSect, bb);
    }

    public static AdfError adfReadBlock(Volume vol, int nSect, ByteBuffer buf)
    {
        int pSect = 0;
        AdfError rc = AdfError.RC_OK;

        if (!vol.mounted)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("the volume isn't mounted, adfReadBlock not possible");
            }
            return AdfError.RC_ERROR;
        }

        /* translate logical sect to physical sect */
        pSect = nSect + vol.firstBlock;

        if (adfEnv != null && adfEnv.useRWAccess && adfEnv.rwhAccess != null)
        {
            adfEnv.rwhAccess.access(pSect, nSect, false);
        }

        if (pSect < vol.firstBlock || pSect > vol.lastBlock)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfReadBlock : nSect out of range");
            }
        }
        rc = vol.dev.adfReadSector(pSect, 512, buf);
        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        } else
        {
            return AdfError.RC_OK;
        }
    }

    /*
     * adfWriteBlock
     *
     */

    public static AdfError adfWriteBlock(Volume vol, int nSect, byte[] buf)
    {
        return adfWriteBlock(vol, nSect, ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN));
    }

    public static AdfError adfWriteBlock(Volume vol, int nSect, ByteBuffer buf)
    {
        int pSect = 0;
        AdfError rc = AdfError.RC_OK;

        if (!vol.mounted)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("the volume isn't mounted, adfWriteBlock not possible");
            }
            return AdfError.RC_ERROR;
        }

        if (vol.readOnly)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWriteBlock : can't write block, read only volume");
            }
            return AdfError.RC_ERROR;
        }

        pSect = nSect + vol.firstBlock;

        if (adfEnv != null && adfEnv.useRWAccess && adfEnv.rwhAccess != null)
        {
            adfEnv.rwhAccess.access(pSect, nSect, true);
        }

        if (pSect < vol.firstBlock || pSect > vol.lastBlock)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWriteBlock : nSect out of range");
            }
        }

        rc = vol.dev.adfWriteSector(pSect, 512, buf);

        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        } else
        {
            return AdfError.RC_OK;
        }
    }

    // ------------------------------------------------------------------
    // Bitmap / helper stubs — kept to preserve adf_disk.c control flow
    // ------------------------------------------------------------------

    private static void adfReadBitmap(Volume vol, int nBlock, BRootBlock root)
    {
        AdfBitm.adfReadBitmap(vol, nBlock, root);
    }

    private static AdfError adfCreateBitmap(Volume vol)
    {
        return AdfBitm.adfCreateBitmap(vol);
    }

    private static void adfGetFreeBlocks(Volume vol, int n, int[] blkList)
    {
        boolean ok = AdfBitm.adfGetFreeBlocks(vol, n, blkList);
        if (!ok)
        {
            if (n > 0 && blkList.length > 0)
            {
                blkList[0] = vol.rootBlock;
            }
            for (int i = 1; i < n && i < blkList.length; i++)
            {
                blkList[i] = vol.rootBlock - i;
                if (blkList[i] < 0)
                {
                    blkList[i] = 0;
                }
            }
        }
    }

    private static void adfCreateEmptyCache(Volume vol, BRootBlock root, int blk)
    {
        // stub
    }

    private static AdfError adfWriteNewBitmap(Volume vol)
    {
        return AdfBitm.adfWriteNewBitmap(vol);
    }

    private static AdfError adfUpdateBitmap(Volume vol)
    {
        return AdfBitm.adfUpdateBitmap(vol);
    }

    private static void adfFreeBitmap(Volume vol)
    {
        AdfBitm.adfFreeBitmap(vol);
    }

    private static int adfCountFreeBlocks(Volume vol)
    {
        return AdfBitm.adfCountFreeBlocks(vol);
    }

    // date helpers — minimal faithful implementations

    private static void adfDays2Date(int days, int[] yy, int[] mm, int[] dd)
    {
        int y = 1978;
        int m = 1;
        int nd = 0;
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if (adfIsLeap(y))
        {
            nd = 366;
        } else
        {
            nd = 365;
        }
        while (days >= nd)
        {
            days -= nd;
            y++;
            if (adfIsLeap(y))
            {
                nd = 366;
            } else
            {
                nd = 365;
            }
        }

        m = 1;
        if (adfIsLeap(y))
        {
            jm[1] = 29;
        }
        while (days >= jm[m - 1])
        {
            days -= jm[m - 1];
            m++;
        }

        yy[0] = y;
        mm[0] = m;
        dd[0] = days + 1;
    }

    private static boolean adfIsLeap(int y)
    {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
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
}
