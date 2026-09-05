/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_hd.c / adf_hd.h + hd_blk.h
 *
 *  $Id$
 *
 *  harddisk / device code
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
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Java port of {@code adf_hd.c} / {@code adf_hd.h} (includes {@code hd_blk.h} handling).
 *
 * <p>Keeps original C control flow and helper names. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF
 * disk structures. Return codes use {@link AdfError} with out-parameters as
 * single-element arrays where the C code used pointer out-params.
 */
public final class AdfHd
{

    private AdfHd()
    {
    }

    /**
     * Global environment — mirrors {@code extern struct Env adfEnv}.
     */
    public static Env adfEnv = AdfRaw.adfEnv;

    /*
     * adfDevType
     *
     * returns the type of a device
     * only based of the field 'dev->size'
     */

    public static int adfDevType(Device dev)
    {
        if ((dev.size == 512 * 11 * 2 * 80) || (dev.size == 512 * 11 * 2 * 81) ||
                (dev.size == 512 * 11 * 2 * 82) || (dev.size == 512 * 11 * 2 * 83))
        {
            return AdfConstants.DEVTYPE_FLOPDD;
        } else if (dev.size == 512 * 22 * 2 * 80)
        {
            return AdfConstants.DEVTYPE_FLOPHD;
        } else if (dev.size > 512 * 22 * 2 * 80)
        {
            return AdfConstants.DEVTYPE_HARDDISK;
        } else
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfDevType : unknown device type");
            }
            return -1;
        }
    }

    /*
     * adfDeviceInfo
     *
     * display information about the device and its volumes
     * for demonstration purpose only since the output is stdout !
     *
     * can be used before adfCreateVol() or adfMount()
     */

    public static void adfDeviceInfo(Device dev)
    {
        int i = 0;

        System.out.printf("Cylinders   = %d\n", dev.cylinders);
        System.out.printf("Heads       = %d\n", dev.heads);
        System.out.printf("Sectors/Cyl = %d\n\n", dev.sectors);
        System.out.printf("Volumes     = %d\n\n", dev.nVol);

        for (i = 0; i < dev.nVol; i++)
        {
            Volume vol = dev.volList.get(i);
            if (vol.volName != null)
            {
                System.out.printf(
                        "%2d :  %7d ->%7d, \"%s\"",
                        i,
                        vol.firstBlock,
                        vol.lastBlock,
                        vol.volName);
            } else
            {
                System.out.printf("%2d :  %7d ->%7d\n", i, vol.firstBlock, vol.lastBlock);
            }
            if (vol.mounted)
            {
                System.out.printf(", mounted");
            }
            System.out.printf("\n");
        }
    }

    /*
     * adfFreeTmpVolList
     *
     */

    public static void adfFreeTmpVolList(List<Volume> list)
    {
        // Java List owns volumes; caller clears list after use.
        if (list != null)
        {
            list.clear();
        }
    }

    /*
     * adfMountHdFile
     *
     */

    public static AdfError adfMountHdFile(Device dev)
    {
        Volume vol = null;
        byte[] buf = new byte[512];
        int size = 0;
        boolean found = false;

        dev.devType = AdfConstants.DEVTYPE_HARDFILE;
        dev.nVol = 0;
        dev.volList = new ArrayList<>();
        vol = new Volume();
        if (vol == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMountHdFile : malloc");
            }
            return AdfError.RC_ERROR;
        }
        dev.volList.add(vol);
        dev.nVol++;

        vol.volName = null;

        dev.cylinders = dev.size / 512;
        dev.heads = 1;
        dev.sectors = 1;

        vol.firstBlock = 0;

        size = dev.size + 512 - (dev.size % 512);
        vol.rootBlock = (size / 512) / 2;
        do
        {
            AdfError rc = dev.adfReadSector(vol.rootBlock, 512, buf);
            if (rc != AdfError.RC_OK)
            {
                break;
            }
            found = AdfEndian.swapLong(buf, 0) == AdfConstants.T_HEADER &&
                    AdfEndian.swapLong(buf, 508) == AdfConstants.ST_ROOT;
            if (!found)
            {
                vol.rootBlock--;
            }
        } while (vol.rootBlock > 1 && !found);

        if (vol.rootBlock == 1)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMountHdFile : rootblock not found");
            }
            return AdfError.RC_ERROR;
        }
        vol.lastBlock = vol.rootBlock * 2 - 1;

        return AdfError.RC_OK;
    }

    /*
     * adfMountHd
     *
     * normal not used directly : called by adfMount()
     *
     * fills geometry fields and volumes list (dev->nVol and dev->volList[])
     */

    public static AdfError adfMountHd(Device dev)
    {
        BRDSKBlock rdsk = new BRDSKBlock();
        BPARTBlock part = new BPARTBlock();
        BFSHDBlock fshd = new BFSHDBlock();
        BLSEGBlock lseg = new BLSEGBlock();
        int next = 0;
        List<Volume> tmpList = new ArrayList<>();
        int i = 0;
        Volume vol = null;
        int len = 0;

        if (adfReadRDSKblock(dev, rdsk) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        dev.cylinders = rdsk.cylinders;
        dev.heads = rdsk.heads;
        dev.sectors = rdsk.sectors;

        /* PART blocks */
        next = rdsk.partitionList;
        dev.nVol = 0;
        while (next != -1)
        {
            if (adfReadPARTblock(dev, next, part) != AdfError.RC_OK)
            {
                adfFreeTmpVolList(tmpList);
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfMountHd : malloc");
                }
                return AdfError.RC_ERROR;
            }

            vol = new Volume();
            if (vol == null)
            {
                adfFreeTmpVolList(tmpList);
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfMountHd : malloc");
                }
                return AdfError.RC_ERROR;
            }
            vol.volName = null;
            dev.nVol++;

            vol.firstBlock = rdsk.cylBlocks * part.lowCyl;
            vol.lastBlock = (part.highCyl + 1) * rdsk.cylBlocks - 1;
            vol.rootBlock = (vol.lastBlock - vol.firstBlock + 1) / 2;
            vol.blockSize = part.blockSize * 4;

            len = AdfConstants.min(31, part.nameLen & 0xFF);
            byte[] nameBytes = new byte[len];
            System.arraycopy(part.name, 0, nameBytes, 0, len);
            vol.volName = new String(nameBytes);

            vol.mounted = false;

            /* stores temporarly the volumes in a linked list */
            tmpList.add(vol);

            next = part.next;
        }

        /* stores the list in an array */
        dev.volList = new ArrayList<>(tmpList);
        dev.nVol = dev.volList.size();

        next = rdsk.fileSysHdrList;
        while (next != -1)
        {
            if (adfReadFSHDblock(dev, next, fshd) != AdfError.RC_OK)
            {
                for (i = 0; i < dev.nVol; i++)
                {
                    // free names not needed in Java GC
                }
                dev.volList.clear();
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfMount : adfReadFSHDblock");
                }
                return AdfError.RC_ERROR;
            }
            next = fshd.next;
        }

        next = fshd.segListBlock;
        while (next != -1)
        {
            if (adfReadLSEGblock(dev, next, lseg) != AdfError.RC_OK)
            {
                if (adfEnv != null && adfEnv.wFct != null)
                {
                    adfEnv.wFct.call("adfMount : adfReadLSEGblock");
                }
            }
            next = lseg.next;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfMountFlop
     *
     * normaly not used directly, called directly by adfMount()
     *
     * use dev->devType to choose between DD and HD
     * fills geometry and the volume list with one volume
     */

    public static AdfError adfMountFlop(Device dev)
    {
        Volume vol = null;
        BRootBlock root = new BRootBlock();
        String diskName = "";

        dev.cylinders = 80;
        dev.heads = 2;
        if (dev.devType == AdfConstants.DEVTYPE_FLOPDD)
        {
            dev.sectors = 11;
        } else
        {
            dev.sectors = 22;
        }

        vol = new Volume();
        if (vol == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMount : malloc");
            }
            return AdfError.RC_ERROR;
        }

        vol.mounted = true;
        vol.firstBlock = 0;
        vol.lastBlock = (dev.cylinders * dev.heads * dev.sectors) - 1;
        vol.rootBlock = (vol.lastBlock + 1 - vol.firstBlock) / 2;
        vol.blockSize = 512;
        vol.dev = dev;

        if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, root) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }
        byte[] nameBytes = new byte[root.nameLen & 0xFF];
        System.arraycopy(root.diskName, 0, nameBytes, 0, nameBytes.length);
        diskName = new String(nameBytes);

        vol.volName = diskName;

        dev.volList = new ArrayList<>();
        if (dev.volList == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMount : malloc");
            }
            return AdfError.RC_ERROR;
        }
        dev.volList.add(vol);
        dev.nVol = 1;

        return AdfError.RC_OK;
    }

    /*
     * adfMountDev
     *
     * mount a dump file (.adf) or a real device (uses adf_nativ.c and .h)
     *
     * adfInitDevice() must fill dev->size !
     */

    public static Device adfMountDev(Device dev)
    {
        AdfError rc = AdfError.RC_OK;
        byte[] buf = new byte[512];

        if (dev == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfMountDev : dev==NULL");
            }
            return null;
        }

        dev.devType = adfDevType(dev);

        switch (dev.devType)
        {

            case AdfConstants.DEVTYPE_FLOPDD:
            case AdfConstants.DEVTYPE_FLOPHD:
                if (adfMountFlop(dev) != AdfError.RC_OK)
                {
                    dev.adfReleaseDevice();
                    return null;
                }
                break;

            case AdfConstants.DEVTYPE_HARDDISK:
                /* to choose between hardfile or harddisk (real or dump) */
                rc = dev.adfReadSector(0, 512, buf);
                if (rc != AdfError.RC_OK)
                {
                    dev.adfReleaseDevice();
                    if (adfEnv != null && adfEnv.eFct != null)
                    {
                        adfEnv.eFct.call("adfMountDev : adfReadSector failed");
                    }
                    return null;
                }

                /* a file with the first three bytes equal to 'DOS' */
                if (buf[0] == 'D' && buf[1] == 'O' && buf[2] == 'S')
                {
                    if (adfMountHdFile(dev) != AdfError.RC_OK)
                    {
                        dev.adfReleaseDevice();
                        return null;
                    }
                } else if (adfMountHd(dev) != AdfError.RC_OK)
                {
                    dev.adfReleaseDevice();
                    return null;
                }
                break;

            default:
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfMountDev : unknown device type");
                }
                dev.adfReleaseDevice();
                return null;
        }

        return dev;
    }

    /**
     * @deprecated file-backed storage removed; create Device subclass and pass to
     * {@link #adfMountDev(Device)}
     */
    @Deprecated
    public static Device adfMountDev(String filename, boolean ro)
    {
        throw new UnsupportedOperationException(
                "file-backed storage removed; create Device subclass and pass to adfMountDev" +
                        "(Device)");
    }

    /*
     * adfCreateHdHeader
     *
     * create PARTIALLY the sectors of the header of one harddisk : can not be mounted
     * back on a real Amiga ! It's because some device dependant values can't be guessed...
     *
     * do not use dev->volList[], but partList for partitions information : start and len are
     * cylinders,
     *  not blocks
     * do not fill dev->volList[]
     * called by adfCreateHd()
     */

    public static AdfError adfCreateHdHeader(Device dev, int n, List<Partition> partList)
    {
        int i = 0;
        BRDSKBlock rdsk = new BRDSKBlock();
        BPARTBlock part = new BPARTBlock();
        BFSHDBlock fshd = new BFSHDBlock();
        BLSEGBlock lseg = new BLSEGBlock();
        int j = 0;
        int len = 0;

        /* RDSK */

        rdsk.rdbBlockLo = 0;
        rdsk.rdbBlockHi = (dev.sectors * dev.heads * 2) - 1;
        rdsk.loCylinder = 2;
        rdsk.hiCylinder = dev.cylinders - 1;
        rdsk.cylBlocks = dev.sectors * dev.heads;

        rdsk.cylinders = dev.cylinders;
        rdsk.sectors = dev.sectors;
        rdsk.heads = dev.heads;

        rdsk.badBlockList = -1;
        rdsk.partitionList = 1;
        rdsk.fileSysHdrList = 1 + dev.nVol;

        if (adfWriteRDSKblock(dev, rdsk) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        /* PART */

        j = 1;
        for (i = 0; i < dev.nVol; i++)
        {
            // clear struct
            part = new BPARTBlock();

            if (i < dev.nVol - 1)
            {
                part.next = j + 1;
            } else
            {
                part.next = -1;
            }

            Partition p = partList.get(i);
            len = AdfConstants.min(AdfConstants.MAXNAMELEN, p.volName.length());
            part.nameLen = (byte) len;
            byte[] nameBytes = p.volName.getBytes();
            System.arraycopy(nameBytes, 0, part.name, 0, len);

            part.surfaces = dev.heads;
            part.blocksPerTrack = dev.sectors;
            part.lowCyl = p.startCyl;
            part.highCyl = p.startCyl + p.lenCyl - 1;
            part.dosType[0] = 'D';
            part.dosType[1] = 'O';
            part.dosType[2] = 'S';

            part.dosType[3] = (byte) (p.volType & 0x01);

            if (adfWritePARTblock(dev, j, part) != AdfError.RC_OK)
            {
                return AdfError.RC_ERROR;
            }
            j++;
        }

        /* FSHD */

        fshd.dosType[0] = 'D';
        fshd.dosType[1] = 'O';
        fshd.dosType[2] = 'S';
        fshd.dosType[3] = (byte) partList.get(0).volType;
        fshd.next = -1;
        fshd.segListBlock = j + 1;
        if (adfWriteFSHDblock(dev, j, fshd) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }
        j++;

        /* LSEG */
        lseg.next = -1;
        if (adfWriteLSEGblock(dev, j, lseg) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCreateFlop
     *
     * create a filesystem on a floppy device
     * fills dev->volList[]
     */

    public static AdfError adfCreateFlop(Device dev, String volName, int volType)
    {
        if (dev == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateFlop : dev==NULL");
            }
            return AdfError.RC_ERROR;
        }
        dev.volList = new ArrayList<>();
        if (dev.volList == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateFlop : unknown device type");
            }
            return AdfError.RC_ERROR;
        }
        Volume v = AdfDisk.adfCreateVol(dev, 0, 80, volName, volType);
        if (v == null)
        {
            return AdfError.RC_ERROR;
        }
        dev.volList.add(v);
        dev.nVol = 1;
        dev.volList.get(0).blockSize = 512;
        if (dev.sectors == 11)
        {
            dev.devType = AdfConstants.DEVTYPE_FLOPDD;
        } else
        {
            dev.devType = AdfConstants.DEVTYPE_FLOPHD;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCreateHd
     *
     * create a filesystem one an harddisk device (partitions==volumes, and the header)
     *
     * fills dev->volList[]
     *
     */

    public static AdfError adfCreateHd(Device dev, int n, List<Partition> partList)
    {
        int i = 0;
        int j = 0;

        if (dev == null || partList == null || n <= 0)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateHd : illegal parameter(s)");
            }
            return AdfError.RC_ERROR;
        }

        dev.volList = new ArrayList<>();
        if (dev.volList == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("adfCreateFlop : malloc");
            }
            return AdfError.RC_ERROR;
        }
        for (i = 0; i < n; i++)
        {
            Partition p = partList.get(i);
            Volume vol = AdfDisk.adfCreateVol(dev, p.startCyl, p.lenCyl, p.volName, p.volType);
            if (vol == null)
            {
                for (j = 0; j < i; j++)
                {
                    // free not needed in Java
                }
                dev.volList.clear();
                if (adfEnv != null && adfEnv.eFct != null)
                {
                    adfEnv.eFct.call("adfCreateHd : adfCreateVol() fails");
                }
                return AdfError.RC_ERROR;
            }
            dev.volList.add(vol);
            dev.volList.get(i).blockSize = 512;
        }
        dev.nVol = n;

        if (adfCreateHdHeader(dev, n, partList) != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }
        return AdfError.RC_OK;
    }

    /*
     * adfUnMountDev
     *
     */

    public static void adfUnMountDev(Device dev)
    {
        int i = 0;

        if (dev == null)
        {
            return;
        }

        for (i = 0; i < dev.nVol; i++)
        {
            // free volName/vol handled by GC
        }
        if (dev.nVol > 0)
        {
            dev.volList.clear();
        }
        dev.nVol = 0;

        dev.adfReleaseDevice();
    }

    /*
     * ReadRDSKblock
     *
     */

    public static AdfError adfReadRDSKblock(Device dev, BRDSKBlock blk)
    {
        byte[] buf = new byte[256];
        AdfError rc2 = AdfError.RC_OK;
        AdfError rc = AdfError.RC_OK;

        rc2 = dev.adfReadSector(0, 256, buf);

        if (rc2 != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BRDSKBlock tmp = BRDSKBlock.read(bb, 0);
        copyRDSK(tmp, blk);

        if (blk.id[0] != 'R' || blk.id[1] != 'D' || blk.id[2] != 'S' || blk.id[3] != 'K')
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("ReadRDSKblock : RDSK id not found");
            }
            return AdfError.RC_ERROR;
        }

        if (blk.size != 64)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadRDSKBlock : size != 64");
            }
        }

        if (blk.checksum != AdfRaw.adfNormalSum(buf, 8, 256))
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadRDSKBlock : incorrect checksum");
            }
        }

        if (blk.blockSize != 512)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadRDSKBlock : blockSize != 512");
            }
        }

        if (blk.cylBlocks != blk.sectors * blk.heads)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadRDSKBlock : cylBlocks != sectors*heads");
            }
        }

        return rc;
    }

    /*
     * adfWriteRDSKblock
     *
     */

    public static AdfError adfWriteRDSKblock(Device dev, BRDSKBlock rdsk)
    {
        byte[] raw = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        ByteBuffer buf = ByteBuffer.wrap(raw).order(ByteOrder.BIG_ENDIAN);
        long newSum = 0;
        AdfError rc2 = AdfError.RC_OK;
        AdfError rc = AdfError.RC_OK;

        if (dev.readOnly)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWriteRDSKblock : can't write block, read only device");
            }
            return AdfError.RC_ERROR;
        }

        Arrays.fill(raw, (byte) 0);

        rdsk.id[0] = 'R';
        rdsk.id[1] = 'D';
        rdsk.id[2] = 'S';
        rdsk.id[3] = 'K';
        rdsk.size = 64;
        rdsk.blockSize = AdfConstants.LOGICAL_BLOCK_SIZE;
        rdsk.badBlockList = -1;

        rdsk.diskVendor[0] = 'A';
        rdsk.diskVendor[1] = 'D';
        rdsk.diskVendor[2] = 'F';
        rdsk.diskVendor[3] = 'l';
        rdsk.diskVendor[4] = 'i';
        rdsk.diskVendor[5] = 'b';
        rdsk.diskVendor[6] = ' ';
        rdsk.diskVendor[7] = ' ';
        byte[] prod = "harddisk.adf    ".getBytes();
        System.arraycopy(
                prod,
                0,
                rdsk.diskProduct,
                0,
                Math.min(prod.length, rdsk.diskProduct.length));
        rdsk.diskRevision[0] = 'v';
        rdsk.diskRevision[1] = '1';
        rdsk.diskRevision[2] = '.';
        rdsk.diskRevision[3] = '0';

        rdsk.write(buf, 0);

        newSum = AdfRaw.adfNormalSum(raw, 8, AdfConstants.LOGICAL_BLOCK_SIZE);
        buf.putInt(8, (int) (newSum & 0xFFFFFFFFL));

        rc2 = dev.adfWriteSector(0, AdfConstants.LOGICAL_BLOCK_SIZE, buf);

        if (rc2 != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return rc;
    }

    /*
     * ReadPARTblock
     *
     */

    public static AdfError adfReadPARTblock(Device dev, int nSect, BPARTBlock blk)
    {
        byte[] buf = new byte[256];
        AdfError rc2 = AdfError.RC_OK;
        AdfError rc = AdfError.RC_OK;

        rc2 = dev.adfReadSector(nSect, 256, buf);

        if (rc2 != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BPARTBlock tmp = BPARTBlock.read(bb, 0);
        copyPART(tmp, blk);

        if (blk.id[0] != 'P' || blk.id[1] != 'A' || blk.id[2] != 'R' || blk.id[3] != 'T')
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("ReadPARTblock : PART id not found");
            }
            return AdfError.RC_ERROR;
        }

        if (blk.size != 64)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadPARTBlock : size != 64");
            }
        }

        if (blk.blockSize != 128)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("ReadPARTblock : blockSize!=512, not supported (yet)");
            }
            return AdfError.RC_ERROR;
        }

        if (blk.checksum != AdfRaw.adfNormalSum(buf, 8, 256))
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadPARTBlock : incorrect checksum");
            }
        }

        return rc;
    }

    /*
     * adfWritePARTblock
     *
     */

    public static AdfError adfWritePARTblock(Device dev, int nSect, BPARTBlock part)
    {
        byte[] raw = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        ByteBuffer buf = ByteBuffer.wrap(raw).order(ByteOrder.BIG_ENDIAN);
        long newSum = 0;
        AdfError rc2 = AdfError.RC_OK;
        AdfError rc = AdfError.RC_OK;

        if (dev.readOnly)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWritePARTblock : can't write block, read only device");
            }
            return AdfError.RC_ERROR;
        }

        Arrays.fill(raw, (byte) 0);

        part.id[0] = 'P';
        part.id[1] = 'A';
        part.id[2] = 'R';
        part.id[3] = 'T';
        part.size = 64;
        part.blockSize = AdfConstants.LOGICAL_BLOCK_SIZE;
        part.vectorSize = 16;
        part.blockSize = 128;
        part.sectorsPerBlock = 1;
        part.dosReserved = 2;

        part.write(buf, 0);

        newSum = AdfRaw.adfNormalSum(raw, 8, AdfConstants.LOGICAL_BLOCK_SIZE);
        buf.putInt(8, (int) (newSum & 0xFFFFFFFFL));

        rc2 = dev.adfWriteSector(nSect, AdfConstants.LOGICAL_BLOCK_SIZE, buf);
        if (rc2 != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return rc;
    }

    /*
     * ReadFSHDblock
     *
     */

    public static AdfError adfReadFSHDblock(Device dev, int nSect, BFSHDBlock blk)
    {
        byte[] buf = new byte[256];
        AdfError rc = AdfError.RC_OK;

        rc = dev.adfReadSector(nSect, 256, buf);
        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BFSHDBlock tmp = BFSHDBlock.read(bb, 0);
        copyFSHD(tmp, blk);

        if (blk.id[0] != 'F' || blk.id[1] != 'S' || blk.id[2] != 'H' || blk.id[3] != 'D')
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("ReadFSHDblock : FSHD id not found");
            }
            return AdfError.RC_ERROR;
        }

        if (blk.size != 64)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadFSHDblock : size != 64");
            }
        }

        if (blk.checksum != AdfRaw.adfNormalSum(buf, 8, 256))
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadFSHDblock : incorrect checksum");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     *  adfWriteFSHDblock
     *
     */

    public static AdfError adfWriteFSHDblock(Device dev, int nSect, BFSHDBlock fshd)
    {
        byte[] raw = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        ByteBuffer buf = ByteBuffer.wrap(raw).order(ByteOrder.BIG_ENDIAN);
        long newSum = 0;
        AdfError rc = AdfError.RC_OK;

        if (dev.readOnly)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWriteFSHDblock : can't write block, read only device");
            }
            return AdfError.RC_ERROR;
        }

        Arrays.fill(raw, (byte) 0);

        fshd.id[0] = 'F';
        fshd.id[1] = 'S';
        fshd.id[2] = 'H';
        fshd.id[3] = 'D';
        fshd.size = 64;

        fshd.write(buf, 0);

        newSum = AdfRaw.adfNormalSum(raw, 8, AdfConstants.LOGICAL_BLOCK_SIZE);
        buf.putInt(8, (int) (newSum & 0xFFFFFFFFL));

        rc = dev.adfWriteSector(nSect, AdfConstants.LOGICAL_BLOCK_SIZE, buf);
        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * ReadLSEGblock
     *
     */

    public static AdfError adfReadLSEGblock(Device dev, int nSect, BLSEGBlock blk)
    {
        byte[] buf = new byte[512];
        AdfError rc = AdfError.RC_OK;

        rc = dev.adfReadSector(nSect, 512, buf);
        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BLSEGBlock tmp = BLSEGBlock.read(bb, 0);
        copyLSEG(tmp, blk);

        if (blk.id[0] != 'L' || blk.id[1] != 'S' || blk.id[2] != 'E' || blk.id[3] != 'G')
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("ReadLSEGblock : LSEG id not found");
            }
            return AdfError.RC_ERROR;
        }

        if (blk.checksum != AdfRaw.adfNormalSum(buf, 8, 512))
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadLSEGBlock : incorrect checksum");
            }
        }

        if (blk.next != -1 && blk.size != 128)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("ReadLSEGBlock : size != 128");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteLSEGblock
     *
     */

    public static AdfError adfWriteLSEGblock(Device dev, int nSect, BLSEGBlock lseg)
    {
        byte[] raw = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        ByteBuffer buf = ByteBuffer.wrap(raw).order(ByteOrder.BIG_ENDIAN);
        long newSum = 0;
        AdfError rc = AdfError.RC_OK;

        if (dev.readOnly)
        {
            if (adfEnv != null && adfEnv.wFct != null)
            {
                adfEnv.wFct.call("adfWriteLSEGblock : can't write block, read only device");
            }
            return AdfError.RC_ERROR;
        }

        Arrays.fill(raw, (byte) 0);

        lseg.id[0] = 'L';
        lseg.id[1] = 'S';
        lseg.id[2] = 'E';
        lseg.id[3] = 'G';
        lseg.size = 128;

        lseg.write(buf, 0);

        newSum = AdfRaw.adfNormalSum(raw, 8, AdfConstants.LOGICAL_BLOCK_SIZE);
        buf.putInt(8, (int) (newSum & 0xFFFFFFFFL));

        rc = dev.adfWriteSector(nSect, AdfConstants.LOGICAL_BLOCK_SIZE, buf);

        if (rc != AdfError.RC_OK)
        {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    // ------------------------------------------------------------------
    // Device-level block I/O — mirrors adfReadBlockDev / adfWriteBlockDev
    // ------------------------------------------------------------------

    public static AdfError adfReadBlockDev(Device dev, int nSect, int size, byte[] buf)
    {
        return dev.adfReadSector(nSect, size, buf);
    }

    public static AdfError adfReadBlockDev(Device dev, int nSect, int size, ByteBuffer buf)
    {
        return dev.adfReadSector(nSect, size, buf);
    }

    public static AdfError adfWriteBlockDev(Device dev, int nSect, int size, byte[] buf)
    {
        return dev.adfWriteSector(nSect, size, buf);
    }

    public static AdfError adfWriteBlockDev(Device dev, int nSect, int size, ByteBuffer buf)
    {
        return dev.adfWriteSector(nSect, size, buf);
    }

    // ------------------------------------------------------------------
    // Dump device helpers — now thin wrappers over abstract Device
    // ------------------------------------------------------------------

    /**
     * @deprecated file-backed storage removed; create Device subclass and pass to
     * {@link #adfMountDev(Device)}
     */
    @Deprecated
    public static boolean adfInitDumpDevice(Device dev, String filename, boolean ro)
    {
        throw new UnsupportedOperationException(
                "file-backed storage removed; create Device subclass and pass to adfMountDev" +
                        "(Device)");
    }

    public static AdfError adfReleaseDumpDevice(Device dev)
    {
        return dev.adfReleaseDevice();
    }

    /**
     * @deprecated file-backed storage removed; create Device subclass directly
     */
    @Deprecated
    public static Device adfCreateDumpDevice(String filename, int cyl, int heads, int sec)
    {
        throw new UnsupportedOperationException(
                "file-backed storage removed; create Device subclass directly");
    }

    public static AdfError adfReadDumpSector(Device dev, int n, int size, byte[] buf)
    {
        return dev.adfReadSector(n, size, buf);
    }

    public static AdfError adfReadDumpSector(Device dev, int n, int size, ByteBuffer buf)
    {
        return dev.adfReadSector(n, size, buf);
    }

    public static AdfError adfWriteDumpSector(Device dev, int n, int size, byte[] buf)
    {
        return dev.adfWriteSector(n, size, buf);
    }

    public static AdfError adfWriteDumpSector(Device dev, int n, int size, ByteBuffer buf)
    {
        return dev.adfWriteSector(n, size, buf);
    }

    /**
     * Wrapper matching original {@code adfCreateHdFile} name used by adf_dump.h.
     */
    public static AdfError adfCreateHdFile(Device dev, String volName, int volType)
    {
        dev.volList = new ArrayList<>();
        Volume vol = AdfDisk.adfCreateVol(dev, 0, dev.cylinders, volName, volType);
        if (vol == null)
        {
            return AdfError.RC_ERROR;
        }
        dev.volList.add(vol);
        dev.nVol = 1;
        return AdfError.RC_OK;
    }

    // ------------------------------------------------------------------
    // Block copy helpers — keep RETCODE pattern faithful to C memcpy blocks
    // ------------------------------------------------------------------

    private static void copyRDSK(BRDSKBlock src, BRDSKBlock dst)
    {
        System.arraycopy(src.id, 0, dst.id, 0, 4);
        dst.size = src.size;
        dst.checksum = src.checksum;
        dst.hostID = src.hostID;
        dst.blockSize = src.blockSize;
        dst.flags = src.flags;
        dst.badBlockList = src.badBlockList;
        dst.partitionList = src.partitionList;
        dst.fileSysHdrList = src.fileSysHdrList;
        dst.driveInit = src.driveInit;
        System.arraycopy(src.r1, 0, dst.r1, 0, src.r1.length);
        dst.cylinders = src.cylinders;
        dst.sectors = src.sectors;
        dst.heads = src.heads;
        dst.interleave = src.interleave;
        dst.parkingZone = src.parkingZone;
        System.arraycopy(src.r2, 0, dst.r2, 0, src.r2.length);
        dst.writePreComp = src.writePreComp;
        dst.reducedWrite = src.reducedWrite;
        dst.stepRate = src.stepRate;
        System.arraycopy(src.r3, 0, dst.r3, 0, src.r3.length);
        dst.rdbBlockLo = src.rdbBlockLo;
        dst.rdbBlockHi = src.rdbBlockHi;
        dst.loCylinder = src.loCylinder;
        dst.hiCylinder = src.hiCylinder;
        dst.cylBlocks = src.cylBlocks;
        dst.autoParkSeconds = src.autoParkSeconds;
        dst.highRDSKBlock = src.highRDSKBlock;
        dst.r4 = src.r4;
        System.arraycopy(src.diskVendor, 0, dst.diskVendor, 0, src.diskVendor.length);
        System.arraycopy(src.diskProduct, 0, dst.diskProduct, 0, src.diskProduct.length);
        System.arraycopy(src.diskRevision, 0, dst.diskRevision, 0, src.diskRevision.length);
        System.arraycopy(
                src.controllerVendor,
                0,
                dst.controllerVendor,
                0,
                src.controllerVendor.length);
        System.arraycopy(
                src.controllerProduct,
                0,
                dst.controllerProduct,
                0,
                src.controllerProduct.length);
        System.arraycopy(
                src.controllerRevision,
                0,
                dst.controllerRevision,
                0,
                src.controllerRevision.length);
        System.arraycopy(src.r5, 0, dst.r5, 0, src.r5.length);
    }

    private static void copyPART(BPARTBlock src, BPARTBlock dst)
    {
        System.arraycopy(src.id, 0, dst.id, 0, 4);
        dst.size = src.size;
        dst.checksum = src.checksum;
        dst.hostID = src.hostID;
        dst.next = src.next;
        dst.flags = src.flags;
        System.arraycopy(src.r1, 0, dst.r1, 0, src.r1.length);
        dst.devFlags = src.devFlags;
        dst.nameLen = src.nameLen;
        System.arraycopy(src.name, 0, dst.name, 0, src.name.length);
        System.arraycopy(src.r2, 0, dst.r2, 0, src.r2.length);
        dst.vectorSize = src.vectorSize;
        dst.blockSize = src.blockSize;
        dst.secOrg = src.secOrg;
        dst.surfaces = src.surfaces;
        dst.sectorsPerBlock = src.sectorsPerBlock;
        dst.blocksPerTrack = src.blocksPerTrack;
        dst.dosReserved = src.dosReserved;
        dst.dosPreAlloc = src.dosPreAlloc;
        dst.interleave = src.interleave;
        dst.lowCyl = src.lowCyl;
        dst.highCyl = src.highCyl;
        dst.numBuffer = src.numBuffer;
        dst.bufMemType = src.bufMemType;
        dst.maxTransfer = src.maxTransfer;
        dst.mask = src.mask;
        dst.bootPri = src.bootPri;
        System.arraycopy(src.dosType, 0, dst.dosType, 0, 4);
        System.arraycopy(src.r3, 0, dst.r3, 0, src.r3.length);
    }

    private static void copyFSHD(BFSHDBlock src, BFSHDBlock dst)
    {
        System.arraycopy(src.id, 0, dst.id, 0, 4);
        dst.size = src.size;
        dst.checksum = src.checksum;
        dst.hostID = src.hostID;
        dst.next = src.next;
        dst.flags = src.flags;
        System.arraycopy(src.r1, 0, dst.r1, 0, src.r1.length);
        System.arraycopy(src.dosType, 0, dst.dosType, 0, 4);
        dst.majVersion = src.majVersion;
        dst.minVersion = src.minVersion;
        dst.patchFlags = src.patchFlags;
        dst.type = src.type;
        dst.task = src.task;
        dst.lock = src.lock;
        dst.handler = src.handler;
        dst.stackSize = src.stackSize;
        dst.priority = src.priority;
        dst.startup = src.startup;
        dst.segListBlock = src.segListBlock;
        dst.globalVec = src.globalVec;
        System.arraycopy(src.r2, 0, dst.r2, 0, src.r2.length);
        System.arraycopy(src.r3, 0, dst.r3, 0, src.r3.length);
    }

    private static void copyLSEG(BLSEGBlock src, BLSEGBlock dst)
    {
        System.arraycopy(src.id, 0, dst.id, 0, 4);
        dst.size = src.size;
        dst.checksum = src.checksum;
        dst.hostID = src.hostID;
        dst.next = src.next;
        System.arraycopy(src.loadData, 0, dst.loadData, 0, src.loadData.length);
    }
}
