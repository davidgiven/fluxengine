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
 * $Id: file.c,v 1.9 1998/11/02 22:08:59 rob Exp $
 */

package org.mars.hfsutils;

import static org.mars.hfsutils.HfsConstants.*;

public final class HfsFile
{
    private HfsFile()
    {
    }

    public static final int FK_DATA = HfsConstants.FK_DATA;
    public static final int FK_RSRC = HfsConstants.FK_RSRC;

    @FunctionalInterface
    interface BlockIO
    {
        int operate(HfsVol vol, int anum, int index, byte[] bp);
    }

    /*
     * NAME:	file->init()
     * DESCRIPTION:	initialize file structure
     */
    public static void f_init(HfsFileHandle file, HfsVol vol,
                              long cnid, String name)
    {
        int i;

        file.vol   = vol;
        file.parid = 0;

        int len = Math.min(name.length(), HFS_MAX_FLEN);
        System.arraycopy(name.toCharArray(), 0, file.name, 0, len);
        file.name[len] = 0;

        file.cat.cdrType          = CatDataType.CDR_FIL_REC;
        file.cat.cdrResrv2        = 0;

        file.cat.filFlags   = 0;
        file.cat.filTyp     = 0;

        file.cat.filUsrWds.fdType       = 0;
        file.cat.filUsrWds.fdCreator    = 0;
        file.cat.filUsrWds.fdFlags      = 0;
        file.cat.filUsrWds.fdLocation.v = 0;
        file.cat.filUsrWds.fdLocation.h = 0;
        file.cat.filUsrWds.fdFldr       = 0;

        file.cat.filFlNum   = cnid;
        file.cat.filStBlk   = 0;
        file.cat.filLgLen   = 0;
        file.cat.filPyLen   = 0;
        file.cat.filRStBlk  = 0;
        file.cat.filRLgLen  = 0;
        file.cat.filRPyLen  = 0;
        file.cat.filCrDat   = 0;
        file.cat.filMdDat   = 0;
        file.cat.filBkDat   = 0;

        file.cat.filFndrInfo.fdIconID = 0;
        for (i = 0; i < 4; ++i)
            file.cat.filFndrInfo.fdUnused[i] = 0;
        file.cat.filFndrInfo.fdComment = 0;
        file.cat.filFndrInfo.fdPutAway = 0;

        file.cat.filClpSize = 0;

        for (i = 0; i < 3; ++i)
        {
            file.cat.filExtRec.data[i].xdrStABN    = 0;
            file.cat.filExtRec.data[i].xdrNumABlks = 0;

            file.cat.filRExtRec.data[i].xdrStABN    = 0;
            file.cat.filRExtRec.data[i].xdrNumABlks = 0;
        }

        file.cat.filResrv   = 0;

        f_selectfork(file, FK_DATA);

        file.flags = 0;

        file.prev  = null;
        file.next  = null;
    }

    /*
     * NAME:	file->selectfork()
     * DESCRIPTION:	choose a fork for file operations
     */
    public static void f_selectfork(HfsFileHandle file, int fork)
    {
        file.fork = fork;

        ExtDataRec src = (fork == FK_DATA)
            ? file.cat.filExtRec
            : file.cat.filRExtRec;

        for (int i = 0; i < 3; ++i)
        {
            file.ext.data[i].xdrStABN    = src.data[i].xdrStABN;
            file.ext.data[i].xdrNumABlks = src.data[i].xdrNumABlks;
        }

        file.fabn = 0;
        file.pos  = 0;
    }

    /*
     * NAME:	file->getptrs()
     * DESCRIPTION:	make pointers to the current fork's lengths and extents
     */
    public static void f_getptrs(HfsFileHandle file,
                                 ExtDataRec[] extrec,
                                 long[] lglen,
                                 long[] pylen)
    {
        if (file.fork == FK_DATA)
        {
            if (extrec != null)
                extrec[0] = file.cat.filExtRec;
            if (lglen != null)
                lglen[0]  = file.cat.filLgLen;
            if (pylen != null)
                pylen[0]  = file.cat.filPyLen;
        }
        else
        {
            if (extrec != null)
                extrec[0] = file.cat.filRExtRec;
            if (lglen != null)
                lglen[0]  = file.cat.filRLgLen;
            if (pylen != null)
                pylen[0]  = file.cat.filRPyLen;
        }
    }

    /*
     * NAME:	file->doblock()
     * DESCRIPTION:	read or write a numbered block from a file
     */
    public static int f_doblock(HfsFileHandle file, long num, byte[] bp,
                                BlockIO func)
    {
        int abnum;
        int blnum;
        int fabn;
        int i;

        abnum = (int) (num / file.vol.lpa);
        blnum = (int) (num % file.vol.lpa);

        /* locate the appropriate extent record */

        fabn = file.fabn;

        if (abnum < fabn)
        {
            ExtDataRec[] extrec = new ExtDataRec[1];

            f_getptrs(file, extrec, null, null);

            fabn = file.fabn = 0;
            for (int j = 0; j < 3; ++j)
            {
                file.ext.data[j].xdrStABN    = extrec[0].data[j].xdrStABN;
                file.ext.data[j].xdrNumABlks = extrec[0].data[j].xdrNumABlks;
            }
        }
        else
            abnum -= fabn;

        while (true)
        {
            int n;

            for (i = 0; i < 3; ++i)
            {
                n = file.ext.data[i].xdrNumABlks & 0xffff;

                if (abnum < n)
                    return func.operate(file.vol,
                                        (file.ext.data[i].xdrStABN & 0xffff) + abnum,
                                        blnum, bp);

                fabn  += n;
                abnum -= n;
            }

            if (HfsVolume.v_extsearch(file, fabn, file.ext, null) <= 0)
                return -1;

            file.fabn = fabn;
        }
    }

    /*
     * NAME:	file->getblock()
     * DESCRIPTION:	read a numbered block from a file
     */
    public static int f_getblock(HfsFileHandle file, long num, byte[] bp)
    {
        return f_doblock(file, num, bp,
                         (vol, anum, idx, buf) ->
                             HfsBlock.b_readab(vol, anum, idx, buf));
    }

    /*
     * NAME:	file->putblock()
     * DESCRIPTION:	write a numbered block to a file
     */
    public static int f_putblock(HfsFileHandle file, long num, byte[] bp)
    {
        return f_doblock(file, num, bp,
                         (vol, anum, idx, buf) ->
                             HfsBlock.b_writeab(vol, anum, idx, buf));
    }

    /*
     * NAME:	file->addextent()
     * DESCRIPTION:	add an extent to a file
     */
    public static int f_addextent(HfsFileHandle file, ExtDescriptor blocks)
    {
        HfsVol vol = file.vol;
        ExtDataRec[] extrec = new ExtDataRec[1];
        long[] pylen = new long[1];
        int start, end;
        Node n = new Node();
        int i;

        f_getptrs(file, extrec, null, pylen);

        start  = file.fabn;
        end    = (int) (pylen[0] / vol.mdb.drAlBlkSiz);

        n.nnum = 0;
        i      = -1;

        while (start < end)
        {
            for (i = 0; i < 3; ++i)
            {
                int num;

                num    = file.ext.data[i].xdrNumABlks & 0xffff;
                start += num;

                if (start == end)
                    break;
                else if (start > end)
                {
                    Hfs.hfsError = "file extents exceed file physical length";
                    Hfs.hfsErrno = HfsException.EIO;
                    return -1;
                }
                else if (num == 0)
                {
                    Hfs.hfsError = "empty file extent";
                    Hfs.hfsErrno = HfsException.EIO;
                    return -1;
                }
            }

            if (start == end)
                break;

            if (HfsVolume.v_extsearch(file, start, file.ext, n) <= 0)
                return -1;

            file.fabn = start;
        }

        if (i >= 0 &&
            (file.ext.data[i].xdrStABN & 0xffff) +
            (file.ext.data[i].xdrNumABlks & 0xffff) ==
            (blocks.xdrStABN & 0xffff))
        {
            file.ext.data[i].xdrNumABlks =
                (short) ((file.ext.data[i].xdrNumABlks & 0xffff) +
                         (blocks.xdrNumABlks & 0xffff));
        }
        else
        {
            /* create a new extent descriptor */

            if (++i < 3)
            {
                file.ext.data[i].xdrStABN    = blocks.xdrStABN;
                file.ext.data[i].xdrNumABlks = blocks.xdrNumABlks;
            }
            else
            {
                ExtKeyRec key = new ExtKeyRec();
                byte[] record = new byte[HFS_MAX_EXTRECLEN];
                int[] reclen = new int[1];

                /* record is full; create a new one */

                file.ext.data[0].xdrStABN    = blocks.xdrStABN;
                file.ext.data[0].xdrNumABlks = blocks.xdrNumABlks;

                for (i = 1; i < 3; ++i)
                {
                    file.ext.data[i].xdrStABN    = 0;
                    file.ext.data[i].xdrNumABlks = 0;
                }

                file.fabn = start;

                HfsRecord.r_makeextkey(key, file.fork,
                                       file.cat.filFlNum, end);
                HfsRecord.r_packextrec(key, file.ext, record, reclen);

                if (HfsBTree.bt_insert(vol.ext, record, reclen[0]) == -1)
                    return -1;

                i = -1;
            }
        }

        if (i >= 0)
        {
            /* store the modified extent record */

            if (file.fabn != 0)
            {
                if ((n.nnum == 0 &&
                     HfsVolume.v_extsearch(file, file.fabn, null, n) <= 0) ||
                    HfsVolume.v_putextrec(file.ext, n) == -1)
                    return -1;
            }
            else
            {
                for (int j = 0; j < 3; ++j)
                {
                    extrec[0].data[j].xdrStABN    = file.ext.data[j].xdrStABN;
                    extrec[0].data[j].xdrNumABlks = file.ext.data[j].xdrNumABlks;
                }
            }
        }

        pylen[0] += (blocks.xdrNumABlks & 0xffff) * vol.mdb.drAlBlkSiz;

        if (file.fork == FK_DATA)
            file.cat.filPyLen = pylen[0];
        else
            file.cat.filRPyLen = pylen[0];

        file.flags |= HFS_FILE_UPDATE_CATREC;

        return 0;
    }

    /*
     * NAME:	file->alloc()
     * DESCRIPTION:	reserve allocation blocks for a file
     */
    public static long f_alloc(HfsFileHandle file)
    {
        HfsVol vol = file.vol;
        long clumpsz;
        ExtDescriptor blocks = new ExtDescriptor();

        clumpsz = file.cat.filClpSize & 0xffff;
        if (clumpsz == 0)
        {
            if (file == vol.ext.f)
                clumpsz = vol.mdb.drXTClpSiz;
            else if (file == vol.cat.f)
                clumpsz = vol.mdb.drCTClpSiz;
            else
                clumpsz = vol.mdb.drClpSiz;
        }

        blocks.xdrNumABlks = (short) (clumpsz / vol.mdb.drAlBlkSiz);

        if (HfsVolume.v_allocblocks(vol, blocks) == -1)
            return -1;

        if (f_addextent(file, blocks) == -1)
        {
            HfsVolume.v_freeblocks(vol, blocks);
            return -1;
        }

        return blocks.xdrNumABlks & 0xffff;
    }

    /*
     * NAME:	file->trunc()
     * DESCRIPTION:	release allocation blocks unneeded by a file
     */
    public static int f_trunc(HfsFileHandle file)
    {
        HfsVol vol = file.vol;
        ExtDataRec[] extrec = new ExtDataRec[1];
        long[] lglen = new long[1];
        long[] pylen = new long[1];
        long alblksz, newpylen;
        int dlen, start, end;
        Node n = new Node();
        int i;

        if ((vol.flags & HFS_VOL_READONLY) != 0)
            return 0;

        f_getptrs(file, extrec, lglen, pylen);

        alblksz  = vol.mdb.drAlBlkSiz;
        newpylen = (lglen[0] / alblksz +
                    (lglen[0] % alblksz != 0 ? 1 : 0)) * alblksz;

        if (newpylen > pylen[0])
        {
            Hfs.hfsError = "file size exceeds physical length";
            Hfs.hfsErrno = HfsException.EIO;
            return -1;
        }
        else if (newpylen == pylen[0])
            return 0;

        dlen  = (int) ((pylen[0] - newpylen) / alblksz);

        start = file.fabn;
        end   = (int) (newpylen / alblksz);

        if (start >= end)
        {
            start = file.fabn = 0;
            for (int j = 0; j < 3; ++j)
            {
                file.ext.data[j].xdrStABN    = extrec[0].data[j].xdrStABN;
                file.ext.data[j].xdrNumABlks = extrec[0].data[j].xdrNumABlks;
            }
        }

        n.nnum = 0;
        i      = -1;

        while (start < end)
        {
            for (i = 0; i < 3; ++i)
            {
                int num;

                num    = file.ext.data[i].xdrNumABlks & 0xffff;
                start += num;

                if (start >= end)
                    break;
                else if (num == 0)
                {
                    Hfs.hfsError = "empty file extent";
                    Hfs.hfsErrno = HfsException.EIO;
                    return -1;
                }
            }

            if (start >= end)
                break;

            if (HfsVolume.v_extsearch(file, start, file.ext, n) <= 0)
                return -1;

            file.fabn = start;
        }

        if (start > end)
        {
            ExtDescriptor blocks = new ExtDescriptor();

            file.ext.data[i].xdrNumABlks =
                (short) ((file.ext.data[i].xdrNumABlks & 0xffff) -
                         (start - end));
            dlen -= start - end;

            blocks.xdrStABN =
                (short) ((file.ext.data[i].xdrStABN & 0xffff) +
                         (file.ext.data[i].xdrNumABlks & 0xffff));
            blocks.xdrNumABlks = (short) (start - end);

            if (HfsVolume.v_freeblocks(vol, blocks) == -1)
                return -1;
        }

        if (file.fork == FK_DATA)
            file.cat.filPyLen = newpylen;
        else
            file.cat.filRPyLen = newpylen;

        file.flags |= HFS_FILE_UPDATE_CATREC;

        do
        {
            while (dlen != 0 && ++i < 3)
            {
                int num;

                num    = file.ext.data[i].xdrNumABlks & 0xffff;
                start += num;

                if (num == 0)
                {
                    Hfs.hfsError = "empty file extent";
                    Hfs.hfsErrno = HfsException.EIO;
                    return -1;
                }
                else if (num > dlen)
                {
                    Hfs.hfsError = "file extents exceed physical size";
                    Hfs.hfsErrno = HfsException.EIO;
                    return -1;
                }

                dlen -= num;

                if (HfsVolume.v_freeblocks(vol, file.ext.data[i]) == -1)
                    return -1;

                file.ext.data[i].xdrStABN    = 0;
                file.ext.data[i].xdrNumABlks = 0;
            }

            if (file.fabn != 0)
            {
                if (n.nnum == 0 &&
                    HfsVolume.v_extsearch(file, file.fabn, null, n) <= 0)
                    return -1;

                if ((file.ext.data[0].xdrNumABlks & 0xffff) != 0)
                {
                    if (HfsVolume.v_putextrec(file.ext, n) == -1)
                        return -1;
                }
                else
                {
                    int recOff = HfsBTree.nodeRec(n, n.rnum);
                    int recLen = HfsBTree.nodeRecLen(n, n.rnum);
                    byte[] rec = new byte[recLen];
                    System.arraycopy(n.data, recOff, rec, 0, recLen);
                    if (HfsBTree.bt_delete(vol.ext, rec) == -1)
                        return -1;

                    n.nnum = 0;
                }
            }
            else
            {
                for (int j = 0; j < 3; ++j)
                {
                    extrec[0].data[j].xdrStABN    = file.ext.data[j].xdrStABN;
                    extrec[0].data[j].xdrNumABlks = file.ext.data[j].xdrNumABlks;
                }
            }

            if (dlen != 0)
            {
                if (HfsVolume.v_extsearch(file, start, file.ext, n) <= 0)
                    return -1;

                file.fabn = start;
                i = -1;
            }
        }
        while (dlen != 0);

        return 0;
    }

    /*
     * NAME:	file->flush()
     * DESCRIPTION:	flush all pending changes to an open file
     */
    public static int f_flush(HfsFileHandle file)
    {
        HfsVol vol = file.vol;

        if ((vol.flags & HFS_VOL_READONLY) != 0)
            return 0;

        if ((file.flags & HFS_FILE_UPDATE_CATREC) != 0)
        {
            Node n = new Node();

            file.cat.filStBlk  = file.cat.filExtRec.data[0].xdrStABN;
            file.cat.filRStBlk = file.cat.filRExtRec.data[0].xdrStABN;

            int nameLen = 0;
            while (nameLen < file.name.length && file.name[nameLen] != 0)
                nameLen++;
            String nameStr = new String(file.name, 0, nameLen);

            if (HfsVolume.v_catsearch(vol, file.parid, nameStr,
                                   null, null, n) <= 0 ||
                HfsVolume.v_putcatrec(file.cat, n) == -1)
                return -1;

            file.flags &= ~HFS_FILE_UPDATE_CATREC;
        }

        return 0;
    }
}
