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
 * $Id: volume.c,v 1.12 1998/11/02 22:09:10 rob Exp $
 */

package org.mars.hfsutils;

import static org.mars.hfsutils.HfsConstants.*;
import static org.mars.hfsutils.HfsException.*;

public final class HfsVolume
{
    private HfsVolume()
    {
    }

    /* Helper methods ========================================================== */

    private static int fail(int errno, String msg)
    {
        Hfs.hfsError = msg;
        Hfs.hfsErrno = errno;
        return -1;
    }

    /*
     * Compute the key-skip offset for a record in a B*-tree node.
     * Equivalent to C macro HFS_RECKEYSKIP(ptr).
     */
    private static int recKeySkip(byte[] data, int recOffset)
    {
        int keyLen = data[recOffset] & 0xff;
        return ((1 + keyLen + 1) & ~1);
    }

    /*
     * Volume bitmap helpers — vbm is byte[][] (array of 512-byte blocks).
     * num is the allocation block number.
     */
    private static boolean bmtst(byte[][] bm, int num)
    {
        int flatByte = num >> 3;

        return (bm[flatByte / HFS_BLOCKSZ][flatByte % HFS_BLOCKSZ]
                & (0x80 >> (num & 0x07))) != 0;
    }

    private static void bmset(byte[][] bm, int num)
    {
        int flatByte = num >> 3;

        bm[flatByte / HFS_BLOCKSZ][flatByte % HFS_BLOCKSZ] |=
            (byte) (0x80 >> (num & 0x07));
    }

    private static void bmclr(byte[][] bm, int num)
    {
        int flatByte = num >> 3;

        bm[flatByte / HFS_BLOCKSZ][flatByte % HFS_BLOCKSZ] &=
            (byte) ~(0x80 >> (num & 0x07));
    }

    /*
     * Compare two MacRoman names stored as byte arrays at given offsets.
     * Equivalent to d_relstring() but operating on byte[] directly.
     */
    private static int compareNames(byte[] name1, int off1,
                                    byte[] name2, int off2)
    {
        for (int i = 0; i < 32; ++i)
        {
            int c1 = name1[off1 + i] & 0xff;
            int c2 = name2[off2 + i] & 0xff;

            if (c1 == 0 && c2 == 0)
                return 0;
            if (c1 == 0)
                return -1;
            if (c2 == 0)
                return 1;

            int diff = (HfsData.HFS_CHARORDER[c1] & 0xff) -
                       (HfsData.HFS_CHARORDER[c2] & 0xff);
            if (diff != 0)
                return diff;
        }
        return 0;
    }

    /*
     * NAME:	vol->init()
     * DESCRIPTION:	initialize volume structure
     */
    public static void v_init(HfsVol vol, int flags)
    {
        BTree ext = vol.ext;
        BTree cat = vol.cat;

        vol.priv       = null;
        vol.flags      = flags & HFS_VOL_OPT_MASK;

        vol.pnum       = -1;
        vol.vstart     = 0;
        vol.vlen       = 0;
        vol.lpa        = 0;

        vol.cache      = null;

        vol.vbm        = null;
        vol.vbmsz      = 0;

        HfsFile.f_init(ext.f, vol, HFS_CNID_EXT, "extents overflow");

        ext.map        = null;
        ext.mapsz      = 0;
        ext.flags      = 0;

        ext.keyunpack  = (pkey, key) -> System.arraycopy(pkey, 0, key, 0,
            Math.min(pkey.length, key.length));
        ext.keycompare = HfsVolume::compareExtKeys;

        HfsFile.f_init(cat.f, vol, HFS_CNID_CAT, "catalog");

        cat.map        = null;
        cat.mapsz      = 0;
        cat.flags      = 0;

        cat.keyunpack  = (pkey, key) -> System.arraycopy(pkey, 0, key, 0,
            Math.min(pkey.length, key.length));
        cat.keycompare = HfsVolume::compareCatKeys;

        vol.cwd        = HFS_CNID_ROOTDIR;

        vol.refs       = 0;
        vol.files      = null;
        vol.dirs       = null;

        vol.prev       = null;
        vol.next       = null;
    }

    /*
     * Compare two unpacked extents keys (stored as packed-format byte arrays).
     * Equivalent to r_compareextkeys() but operating on byte[].
     *
     * Packed layout: keyLen(1) + fkType(1) + fNum(4) + fABN(2)
     * Comparison order: fNum, fkType, fABN
     */
    private static int compareExtKeys(byte[] key1, byte[] key2)
    {
        long fNum1 = HfsData.d_getul(key1, 2);
        long fNum2 = HfsData.d_getul(key2, 2);

        if (fNum1 != fNum2)
            return fNum1 < fNum2 ? -1 : 1;

        int diff = (key1[1] & 0xff) - (key2[1] & 0xff);
        if (diff != 0)
            return diff;

        int fABN1 = HfsData.d_getuw(key1, 6);
        int fABN2 = HfsData.d_getuw(key2, 6);
        return fABN1 - fABN2;
    }

    /*
     * Compare two unpacked catalog keys (stored as packed-format byte arrays).
     * Equivalent to r_comparecatkeys() but operating on byte[].
     *
     * Packed layout: keyLen(1) + resrv1(1) + parID(4) + cName(32)
     * Comparison order: parID, cName
     */
    private static int compareCatKeys(byte[] key1, byte[] key2)
    {
        long parID1 = HfsData.d_getul(key1, 2);
        long parID2 = HfsData.d_getul(key2, 2);

        if (parID1 != parID2)
            return parID1 < parID2 ? -1 : 1;

        return compareNames(key1, 6, key2, 6);
    }

    /*
     * NAME:	vol->open()
     * DESCRIPTION:	open volume source and lock against concurrent updates
     */
    public static int v_open(HfsVol vol, String path, int mode)
    {
        if ((vol.flags & HFS_VOL_OPEN) != 0)
            return fail(EINVAL, "volume already open");

        if (vol.priv.open(path, mode) == -1)
            return -1;

        vol.flags |= HFS_VOL_OPEN;

        /* initialize volume block cache (OK to fail) */

        if ((vol.flags & HFS_OPT_NOCACHE) == 0 &&
            HfsBlock.b_init(vol) != -1)
            vol.flags |= HFS_VOL_USINGCACHE;

        return 0;
    }

    /*
     * NAME:	flushvol()
     * DESCRIPTION:	flush all pending changes (B*-tree, MDB, VBM) to volume
     */
    private static int flushvol(HfsVol vol, boolean umount)
    {
        if ((vol.flags & HFS_VOL_READONLY) != 0)
            return 0;

        if ((vol.ext.flags & HFS_BT_UPDATE_HDR) != 0 &&
            HfsBTree.bt_writehdr(vol.ext) == -1)
            return -1;

        if ((vol.cat.flags & HFS_BT_UPDATE_HDR) != 0 &&
            HfsBTree.bt_writehdr(vol.cat) == -1)
            return -1;

        if ((vol.flags & HFS_VOL_UPDATE_VBM) != 0 &&
            v_writevbm(vol) == -1)
            return -1;

        if (umount && (vol.mdb.drAtrb & HFS_ATRB_UMOUNTED) == 0)
        {
            vol.mdb.drAtrb |= HFS_ATRB_UMOUNTED;
            vol.flags |= HFS_VOL_UPDATE_MDB;
        }

        if ((vol.flags & (HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_ALTMDB)) != 0 &&
            v_writemdb(vol) == -1)
            return -1;

        return 0;
    }

    /*
     * NAME:	vol->flush()
     * DESCRIPTION:	commit all pending changes to volume device
     */
    public static int v_flush(HfsVol vol)
    {
        if (flushvol(vol, false) == -1)
            return -1;

        if ((vol.flags & HFS_VOL_USINGCACHE) != 0 &&
            HfsBlock.b_flush(vol) == -1)
            return -1;

        return 0;
    }

    /*
     * NAME:	vol->close()
     * DESCRIPTION:	close access path to volume source
     */
    public static int v_close(HfsVol vol)
    {
        int result = 0;

        if ((vol.flags & HFS_VOL_OPEN) == 0)
            return 0;

        if ((vol.flags & HFS_VOL_MOUNTED) != 0 &&
            flushvol(vol, true) == -1)
            result = -1;

        if ((vol.flags & HFS_VOL_USINGCACHE) != 0 &&
            HfsBlock.b_finish(vol) == -1)
            result = -1;

        if (vol.priv.close() == -1)
            result = -1;

        vol.flags &= ~(HFS_VOL_OPEN | HFS_VOL_MOUNTED | HFS_VOL_USINGCACHE);

        /* free dynamically allocated structures */

        vol.vbm   = null;
        vol.vbmsz = 0;

        vol.ext.map = null;
        vol.cat.map = null;

        return result;
    }

    /*
     * NAME:	vol->same()
     * DESCRIPTION:	return non-zero iff path is same as open volume
     */
    public static int v_same(HfsVol vol, String path)
    {
        return vol.priv.same(path);
    }

    /*
     * NAME:	vol->geometry()
     * DESCRIPTION:	determine volume location and size (possibly in a partition)
     */
    public static int v_geometry(HfsVol vol, int pnum)
    {
        Partition map = new Partition();
        long bnum = 0;
        int found;

        vol.pnum = pnum;

        if (pnum == 0)
        {
            vol.vstart = 0;
            vol.vlen   = HfsBlock.b_size(vol);

            if (vol.vlen == 0)
                return -1;
        }
        else
        {
            int remaining = pnum;

            while (remaining-- > 0)
            {
                found = HfsMedium.m_findpmentry(vol, "Apple_HFS", map,
                                                 new long[]{ bnum });
                if (found == -1 || found == 0)
                    return -1;

                bnum++;
            }

            vol.vstart = map.pmPyPartStart;
            vol.vlen   = map.pmPartBlkCnt;

            if (map.pmDataCnt != 0)
            {
                if ((long) map.pmLgDataStart +
                    (long) map.pmDataCnt > vol.vlen)
                    return fail(EINVAL, "partition data overflows partition");

                vol.vstart += (long) map.pmLgDataStart;
                vol.vlen    = map.pmDataCnt;
            }

            if (vol.vlen == 0)
                return fail(EINVAL, "volume partition is empty");
        }

        if (vol.vlen < 800 * (1024 >> HFS_BLOCKSZ_BITS))
            return fail(EINVAL, "volume is smaller than 800K");

        return 0;
    }

    /*
     * NAME:	vol->readmdb()
     * DESCRIPTION:	load Master Directory Block into memory
     */
    public static int v_readmdb(HfsVol vol)
    {
        if (HfsLow.l_getmdb(vol, vol.mdb, false) == -1)
            return -1;

        if (vol.mdb.drSigWord != HFS_SIGWORD)
        {
            if (vol.mdb.drSigWord == HFS_SIGWORD_MFS)
                return fail(EINVAL, "MFS volume format not supported");
            else
                return fail(EINVAL, "not a Macintosh HFS volume");
        }

        if (vol.mdb.drAlBlkSiz % HFS_BLOCKSZ != 0)
            return fail(EINVAL, "bad volume allocation block size");

        vol.lpa = (int) (vol.mdb.drAlBlkSiz >> HFS_BLOCKSZ_BITS);

        /* extents pseudo-file structs */

        vol.ext.f.cat.filStBlk = vol.mdb.drXTExtRec.data[0].xdrStABN;
        vol.ext.f.cat.filLgLen = vol.mdb.drXTFlSize;
        vol.ext.f.cat.filPyLen = vol.mdb.drXTFlSize;

        vol.ext.f.cat.filCrDat = vol.mdb.drCrDate;
        vol.ext.f.cat.filMdDat = vol.mdb.drLsMod;

        for (int i = 0; i < 3; ++i)
        {
            vol.ext.f.cat.filExtRec.data[i].xdrStABN    = vol.mdb.drXTExtRec.data[i].xdrStABN;
            vol.ext.f.cat.filExtRec.data[i].xdrNumABlks = vol.mdb.drXTExtRec.data[i].xdrNumABlks;
        }

        HfsFile.f_selectfork(vol.ext.f, FK_DATA);

        /* catalog pseudo-file structs */

        vol.cat.f.cat.filStBlk = vol.mdb.drCTExtRec.data[0].xdrStABN;
        vol.cat.f.cat.filLgLen = vol.mdb.drCTFlSize;
        vol.cat.f.cat.filPyLen = vol.mdb.drCTFlSize;

        vol.cat.f.cat.filCrDat = vol.mdb.drCrDate;
        vol.cat.f.cat.filMdDat = vol.mdb.drLsMod;

        for (int i = 0; i < 3; ++i)
        {
            vol.cat.f.cat.filExtRec.data[i].xdrStABN    = vol.mdb.drCTExtRec.data[i].xdrStABN;
            vol.cat.f.cat.filExtRec.data[i].xdrNumABlks = vol.mdb.drCTExtRec.data[i].xdrNumABlks;
        }

        HfsFile.f_selectfork(vol.cat.f, FK_DATA);

        return 0;
    }

    /*
     * NAME:	vol->writemdb()
     * DESCRIPTION:	flush Master Directory Block to medium
     */
    public static int v_writemdb(HfsVol vol)
    {
        vol.mdb.drLsMod = HfsData.d_mtime(System.currentTimeMillis() / 1000);

        vol.mdb.drXTFlSize = vol.ext.f.cat.filPyLen;
        for (int i = 0; i < 3; ++i)
        {
            vol.mdb.drXTExtRec.data[i].xdrStABN    = vol.ext.f.cat.filExtRec.data[i].xdrStABN;
            vol.mdb.drXTExtRec.data[i].xdrNumABlks = vol.ext.f.cat.filExtRec.data[i].xdrNumABlks;
        }

        vol.mdb.drCTFlSize = vol.cat.f.cat.filPyLen;
        for (int i = 0; i < 3; ++i)
        {
            vol.mdb.drCTExtRec.data[i].xdrStABN    = vol.cat.f.cat.filExtRec.data[i].xdrStABN;
            vol.mdb.drCTExtRec.data[i].xdrNumABlks = vol.cat.f.cat.filExtRec.data[i].xdrNumABlks;
        }

        if (HfsLow.l_putmdb(vol, vol.mdb,
                             (vol.flags & HFS_VOL_UPDATE_ALTMDB) != 0) == -1)
            return -1;

        vol.flags &= ~(HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_ALTMDB);

        return 0;
    }

    /*
     * NAME:	vol->readvbm()
     * DESCRIPTION:	read volume bitmap into memory
     */
    public static int v_readvbm(HfsVol vol)
    {
        int vbmst = vol.mdb.drVBMSt & 0xffff;
        int vbmsz = ((vol.mdb.drNmAlBlks & 0xffff) + 0x0fff) >> 12;

        /* ASSERT(vol.vbm == null); */

        if ((vol.mdb.drAlBlSt & 0xffff) - vbmst < vbmsz)
            return fail(EIO, "volume bitmap collides with volume data");

        vol.vbm = new byte[vbmsz][];

        for (int i = 0; i < vbmsz; ++i)
            vol.vbm[i] = new byte[HFS_BLOCKSZ];

        vol.vbmsz = (short) vbmsz;

        for (int i = 0; i < vbmsz; ++i)
        {
            if (HfsLow.b_readlb(vol, vbmst + i, vol.vbm[i]) == -1)
            {
                vol.vbm   = null;
                vol.vbmsz = 0;
                return -1;
            }
        }

        return 0;
    }

    /*
     * NAME:	vol->writevbm()
     * DESCRIPTION:	flush volume bitmap to medium
     */
    public static int v_writevbm(HfsVol vol)
    {
        int vbmst = vol.mdb.drVBMSt & 0xffff;
        int vbmsz = vol.vbmsz & 0xffff;

        for (int i = 0; i < vbmsz; ++i)
        {
            if (HfsLow.b_writelb(vol, vbmst + i, vol.vbm[i]) == -1)
                return -1;
        }

        vol.flags &= ~HFS_VOL_UPDATE_VBM;

        return 0;
    }

    /*
     * NAME:	vol->mount()
     * DESCRIPTION:	load volume information into memory
     */
    public static int v_mount(HfsVol vol)
    {
        /* read the MDB, volume bitmap, and extents/catalog B*-tree headers */

        if (v_readmdb(vol) == -1 ||
            v_readvbm(vol) == -1 ||
            HfsBTree.bt_readhdr(vol.ext) == -1 ||
            HfsBTree.bt_readhdr(vol.cat) == -1)
            return -1;

        if ((vol.mdb.drAtrb & HFS_ATRB_UMOUNTED) == 0 &&
            v_scavenge(vol) == -1)
            return -1;

        if ((vol.mdb.drAtrb & HFS_ATRB_SLOCKED) != 0)
            vol.flags |= HFS_VOL_READONLY;
        else if ((vol.flags & HFS_VOL_READONLY) != 0)
            vol.mdb.drAtrb |= HFS_ATRB_HLOCKED;
        else
            vol.mdb.drAtrb &= ~HFS_ATRB_HLOCKED;

        vol.flags |= HFS_VOL_MOUNTED;

        return 0;
    }

    /*
     * NAME:	vol->dirty()
     * DESCRIPTION:	ensure the volume is marked "in use" before we make changes
     */
    public static int v_dirty(HfsVol vol)
    {
        if ((vol.mdb.drAtrb & HFS_ATRB_UMOUNTED) != 0)
        {
            vol.mdb.drAtrb &= ~HFS_ATRB_UMOUNTED;
            ++vol.mdb.drWrCnt;

            if (v_writemdb(vol) == -1)
                return -1;

            if ((vol.flags & HFS_VOL_USINGCACHE) != 0 &&
                HfsBlock.b_flush(vol) == -1)
                return -1;
        }

        return 0;
    }

    /*
     * NAME:	vol->catsearch()
     * DESCRIPTION:	search catalog tree
     */
    public static int v_catsearch(HfsVol vol, long parid, String name,
                                  CatDataRec data, char[] cname, Node np)
    {
        CatKeyRec key = new CatKeyRec();
        byte[] pkey = new byte[HFS_CATKEYLEN];
        int found;

        if (np == null)
            np = new Node();

        HfsRecord.r_makecatkey(key, parid, name);
        HfsRecord.r_packcatkey(key, pkey, null);

        found = HfsBTree.bt_search(vol.cat, pkey, np);
        if (found <= 0)
            return found;

        int recOff = np.roff[np.rnum];
        int keySkip = recKeySkip(np.data, recOff);

        if (cname != null)
        {
            HfsRecord.r_unpackcatkey(np.data, recOff, key);

            int len = 0;
            while (len < key.ckrCName.length && key.ckrCName[len] != 0)
                ++len;
            System.arraycopy(key.ckrCName, 0, cname, 0, len);
            cname[len] = 0;
        }

        if (data != null)
            HfsRecord.r_unpackcatdata(np.data, recOff + keySkip, data);

        return 1;
    }

    /*
     * NAME:	vol->extsearch()
     * DESCRIPTION:	search extents tree
     */
    public static int v_extsearch(HfsFileHandle file, int fabn,
                                  ExtDataRec data, Node np)
    {
        ExtKeyRec key = new ExtKeyRec();
        ExtDataRec extsave = new ExtDataRec();
        int fabnsave;
        byte[] pkey = new byte[HFS_EXTKEYLEN];
        int found;

        if (np == null)
            np = new Node();

        HfsRecord.r_makeextkey(key, file.fork,
                               file.cat.filFlNum, fabn);
        HfsRecord.r_packextkey(key, pkey, null);

        /* in case bt_search() clobbers these */

        for (int i = 0; i < 3; ++i)
        {
            extsave.data[i].xdrStABN    = file.ext.data[i].xdrStABN;
            extsave.data[i].xdrNumABlks = file.ext.data[i].xdrNumABlks;
        }
        fabnsave = file.fabn;

        found = HfsBTree.bt_search(file.vol.ext, pkey, np);

        for (int i = 0; i < 3; ++i)
        {
            file.ext.data[i].xdrStABN    = extsave.data[i].xdrStABN;
            file.ext.data[i].xdrNumABlks = extsave.data[i].xdrNumABlks;
        }
        file.fabn = fabnsave;

        if (found <= 0)
            return found;

        if (data != null)
        {
            int recOff = np.roff[np.rnum];
            int keySkip = recKeySkip(np.data, recOff);
            HfsRecord.r_unpackextdata(np.data, recOff + keySkip, data);
        }

        return 1;
    }

    /*
     * NAME:	vol->getthread()
     * DESCRIPTION:	retrieve catalog thread information for a file or directory
     */
    public static int v_getthread(HfsVol vol, long id,
                                  CatDataRec thread, Node np, int type)
    {
        CatDataRec rec;
        int found;

        if (thread == null)
            rec = new CatDataRec();
        else
            rec = thread;

        found = v_catsearch(vol, id, "", rec, null, np);
        if (found == 1 && rec.cdrType != type)
            return fail(EIO, "bad thread record");

        return found;
    }

    /*
     * NAME:	vol->putcatrec()
     * DESCRIPTION:	store catalog information
     */
    public static int v_putcatrec(CatDataRec data, Node np)
    {
        byte[] pdata = new byte[HFS_MAX_DATALEN];
        int[] len = {0};

        HfsRecord.r_packcatdata(data, pdata, len);

        int recOff = np.roff[np.rnum];
        int keySkip = recKeySkip(np.data, recOff);
        System.arraycopy(pdata, 0, np.data, recOff + keySkip, len[0]);

        return HfsBTree.bt_putnode(np);
    }

    /*
     * NAME:	vol->putextrec()
     * DESCRIPTION:	store extent information
     */
    public static int v_putextrec(ExtDataRec data, Node np)
    {
        byte[] pdata = new byte[HFS_MAX_DATALEN];
        int[] len = {0};

        HfsRecord.r_packextdata(data, pdata, len);

        int recOff = np.roff[np.rnum];
        int keySkip = recKeySkip(np.data, recOff);
        System.arraycopy(pdata, 0, np.data, recOff + keySkip, len[0]);

        return HfsBTree.bt_putnode(np);
    }

    /*
     * NAME:	vol->allocblocks()
     * DESCRIPTION:	allocate a contiguous range of blocks
     */
    public static int v_allocblocks(HfsVol vol, ExtDescriptor blocks)
    {
        int request, found, foundat, start, end;
        int pt;
        byte[][] vbm;
        boolean wrap = false;

        if ((vol.mdb.drFreeBks & 0xffff) == 0)
            return fail(ENOSPC, "volume full");

        request = blocks.xdrNumABlks & 0xffff;
        found   = 0;
        foundat = 0;
        start   = vol.mdb.drAllocPtr & 0xffff;
        end     = vol.mdb.drNmAlBlks & 0xffff;
        vbm     = vol.vbm;

        /* backtrack the start pointer to recover unused space */

        if (! bmtst(vbm, start))
        {
            while (start > 0 && ! bmtst(vbm, start - 1))
                --start;
        }

        /* find largest unused block which satisfies request */

        pt = start;

        while (true)
        {
            int mark;

            /* skip blocks in use */

            while (pt < end && bmtst(vbm, pt))
                ++pt;

            if (wrap && pt >= start)
                break;

            /* count blocks not in use */

            mark = pt;
            while (pt < end && pt - mark < request && ! bmtst(vbm, pt))
                ++pt;

            if (pt - mark > found)
            {
                found   = pt - mark;
                foundat = mark;
            }

            if (wrap && pt >= start)
                break;

            if (pt == end)
            {
                pt = 0;
                wrap = true;
            }

            if (found == request)
                break;
        }

        if (found == 0 || found > (vol.mdb.drFreeBks & 0xffff))
            return fail(EIO, "bad volume bitmap or free block count");

        blocks.xdrStABN    = (short) foundat;
        blocks.xdrNumABlks = (short) found;

        if (v_dirty(vol) == -1)
            return -1;

        vol.mdb.drAllocPtr = (short) pt;
        vol.mdb.drFreeBks  = (short) ((vol.mdb.drFreeBks & 0xffff) - found);

        for (pt = foundat; pt < foundat + found; ++pt)
            bmset(vbm, pt);

        vol.flags |= HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_VBM;

        if ((vol.flags & HFS_OPT_ZERO) != 0)
        {
            byte[] b = new byte[HFS_BLOCKSZ];

            for (pt = foundat; pt < foundat + found; ++pt)
            {
                for (int i = 0; i < vol.lpa; ++i)
                    HfsBlock.b_writeab(vol, pt, i, b);
            }
        }

        return 0;
    }

    /*
     * NAME:	vol->freeblocks()
     * DESCRIPTION:	deallocate a contiguous range of blocks
     */
    public static int v_freeblocks(HfsVol vol, ExtDescriptor blocks)
    {
        int start, len, pt;
        byte[][] vbm;

        start = blocks.xdrStABN & 0xffff;
        len   = blocks.xdrNumABlks & 0xffff;
        vbm   = vol.vbm;

        if (v_dirty(vol) == -1)
            return -1;

        vol.mdb.drFreeBks = (short) ((vol.mdb.drFreeBks & 0xffff) + len);

        for (pt = start; pt < start + len; ++pt)
            bmclr(vbm, pt);

        vol.flags |= HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_VBM;

        return 0;
    }

    /*
     * NAME:	vol->resolve()
     * DESCRIPTION:	translate a pathname; return catalog information
     *
     * NOTE: vol is passed as HfsVol[] because the path can change the
     * current volume (absolute paths with volume name).
     */
    public static int v_resolve(HfsVol[] vol, String path,
                                CatDataRec data, long[] parid,
                                char[] fname, Node np)
    {
        long dirid;
        int found = 0;

        if (path.isEmpty())
            return fail(ENOENT, "empty path");

        if (parid != null)
            parid[0] = 0;

        int colonIdx = path.indexOf(':');

        if (path.charAt(0) == ':' || colonIdx == -1)
        {
            dirid = vol[0].cwd;  /* relative path */

            if (path.charAt(0) == ':')
                path = path.substring(1);

            if (path.isEmpty())
            {
                found = v_getthread(vol[0], dirid, data, null,
                                    CatDataType.CDR_THD_REC);
                if (found == -1)
                    return -1;

                if (found == 1)
                {
                    if (parid != null)
                        parid[0] = data.thdParID;

                    found = v_catsearch(vol[0], data.thdParID,
                                        new String(data.thdCName),
                                        data, fname, np);
                    if (found == -1)
                        return -1;
                }

                return found;
            }
        }
        else
        {
            dirid = HFS_CNID_ROOTPAR;  /* absolute path */

            if (colonIdx > HFS_MAX_VLEN)
                return fail(ENAMETOOLONG, null);

            String volName = path.substring(0, colonIdx);
            path = path.substring(colonIdx);

            /* search mounted volumes for matching name */
            for (HfsVol check = Hfs.hfsMounts; check != null; check = check.next)
            {
                if (HfsData.d_relstring(check.mdb.drVN,
                                        volName.toCharArray()) == 0)
                {
                    vol[0] = check;
                    break;
                }
            }
        }

        while (true)
        {
            while (!path.isEmpty() && path.charAt(0) == ':')
            {
                path = path.substring(1);

                found = v_getthread(vol[0], dirid, data, null,
                                    CatDataType.CDR_THD_REC);
                if (found == -1)
                    return -1;
                else if (found == 0)
                    return found;

                dirid = data.thdParID;
            }

            if (path.isEmpty())
            {
                found = v_getthread(vol[0], dirid, data, null,
                                    CatDataType.CDR_THD_REC);
                if (found == -1)
                    return -1;

                if (found == 1)
                {
                    if (parid != null)
                        parid[0] = data.thdParID;

                    found = v_catsearch(vol[0], data.thdParID,
                                        new String(data.thdCName),
                                        data, fname, np);
                    if (found == -1)
                        return -1;
                }

                return found;
            }

            /* extract next component (up to HFS_MAX_FLEN chars) */
            StringBuilder componentName = new StringBuilder();
            int i = 0;
            while (i < path.length() && path.charAt(i) != ':'
                   && componentName.length() < HFS_MAX_FLEN)
            {
                componentName.append(path.charAt(i));
                ++i;
            }

            if (i < path.length() && path.charAt(i) != ':')
                return fail(ENAMETOOLONG, null);

            String name = componentName.toString();
            if (i < path.length() && path.charAt(i) == ':')
                ++i;

            path = path.substring(i);

            if (parid != null)
                parid[0] = dirid;

            found = v_catsearch(vol[0], dirid, name, data, fname, np);
            if (found == -1)
                return -1;

            if (found == 0)
            {
                if (!path.isEmpty() && parid != null)
                    parid[0] = 0;

                if (path.isEmpty() && fname != null)
                {
                    int len = Math.min(name.length(), fname.length);
                    name.getChars(0, len, fname, 0);
                    fname[len] = 0;
                }

                return found;
            }

            switch (data.cdrType)
            {
            case CatDataType.CDR_DIR_REC:
                if (path.isEmpty())
                    return found;

                dirid = data.dirDirID;
                break;

            case CatDataType.CDR_FIL_REC:
                if (path.isEmpty())
                    return found;

                return fail(ENOTDIR, "invalid pathname");

            default:
                return fail(EIO, "unexpected catalog record");
            }
        }
    }

    /*
     * NAME:	vol->adjvalence()
     * DESCRIPTION:	update a volume's valence counts
     */
    public static int v_adjvalence(HfsVol vol, long parid,
                                   boolean isdir, int adj)
    {
        Node n = new Node();
        CatDataRec data = new CatDataRec();
        int result = 0;

        if (isdir)
            vol.mdb.drDirCnt = vol.mdb.drDirCnt + adj;
        else
            vol.mdb.drFilCnt = vol.mdb.drFilCnt + adj;

        vol.flags |= HFS_VOL_UPDATE_MDB;

        if (parid == HFS_CNID_ROOTDIR)
        {
            if (isdir)
                vol.mdb.drNmRtDirs =
                    (short) ((vol.mdb.drNmRtDirs & 0xffff) + adj);
            else
                vol.mdb.drNmFls =
                    (short) ((vol.mdb.drNmFls & 0xffff) + adj);
        }
        else if (parid == HFS_CNID_ROOTPAR)
            return 0;

        if (v_getthread(vol, parid, data, null, CatDataType.CDR_THD_REC) <= 0 ||
            v_catsearch(vol, data.thdParID, new String(data.thdCName),
                        data, null, n) <= 0 ||
            data.cdrType != CatDataType.CDR_DIR_REC)
            return fail(EIO, "can't find parent directory");

        data.dirVal  = (short) ((data.dirVal & 0xffff) + adj);
        data.dirMdDat = HfsData.d_mtime(System.currentTimeMillis() / 1000);

        result = v_putcatrec(data, n);

        return result;
    }

    /*
     * NAME:	vol->mkdir()
     * DESCRIPTION:	create a new HFS directory
     */
    public static int v_mkdir(HfsVol vol, long parid, String name)
    {
        CatKeyRec key = new CatKeyRec();
        CatDataRec data = new CatDataRec();
        long id;
        byte[] record = new byte[HFS_MAX_CATRECLEN];
        int[] reclen = new int[1];

        if (HfsBTree.bt_space(vol.cat, 2) == -1)
            return -1;

        id = vol.mdb.drNxtCNID++;
        vol.flags |= HFS_VOL_UPDATE_MDB;

        /* create directory record */

        data.cdrType   = CatDataType.CDR_DIR_REC;
        data.cdrResrv2 = 0;

        data.dirFlags = 0;
        data.dirVal   = 0;
        data.dirDirID = id;
        data.dirCrDat = HfsData.d_mtime(System.currentTimeMillis() / 1000);
        data.dirMdDat = data.dirCrDat;
        data.dirBkDat = 0;

        data.dirUsrInfo  = new DInfo();
        data.dirFndrInfo = new DXInfo();
        for (int i = 0; i < 4; ++i)
            data.dirResrv[i] = 0;

        HfsRecord.r_makecatkey(key, parid, name);
        HfsRecord.r_packcatrec(key, data, record, reclen);

        if (HfsBTree.bt_insert(vol.cat, record, reclen[0]) == -1)
            return -1;

        /* create thread record */

        data.cdrType   = CatDataType.CDR_THD_REC;
        data.cdrResrv2 = 0;

        data.thdResrv[0] = 0;
        data.thdResrv[1] = 0;
        data.thdParID    = parid;

        int nameLen = Math.min(name.length(), 31);
        for (int j = 0; j < nameLen; ++j)
            data.thdCName[j] = name.charAt(j);
        data.thdCName[nameLen] = 0;

        HfsRecord.r_makecatkey(key, id, "");
        HfsRecord.r_packcatrec(key, data, record, reclen);

        if (HfsBTree.bt_insert(vol.cat, record, reclen[0]) == -1 ||
            v_adjvalence(vol, parid, true, 1) == -1)
            return -1;

        return 0;
    }

    /*
     * NAME:	markexts()
     * DESCRIPTION:	set bits from an extent record in the volume bitmap
     */
    private static void markexts(byte[][] vbm, ExtDataRec exts)
    {
        for (int i = 0; i < 3; ++i)
        {
            int pt  = exts.data[i].xdrStABN & 0xffff;
            int len = exts.data[i].xdrNumABlks & 0xffff;

            while (len-- > 0)
                bmset(vbm, pt++);
        }
    }

    /*
     * NAME:	vol->scavenge()
     * DESCRIPTION:	safeguard blocks in the volume bitmap
     */
    public static int v_scavenge(HfsVol vol)
    {
        byte[][] vbm = vol.vbm;
        Node n = new Node();
        int pt;
        long lastcnid = 15;

        if ((vol.flags & HFS_VOL_READONLY) != 0)
            return 0;

        /* reset MDB by marking it dirty again */

        vol.mdb.drAtrb |= HFS_ATRB_UMOUNTED;
        if (v_dirty(vol) == -1)
            return -1;

        /* begin by marking extents in MDB */

        markexts(vbm, vol.mdb.drXTExtRec);
        markexts(vbm, vol.mdb.drCTExtRec);

        vol.flags |= HFS_VOL_UPDATE_VBM;

        /* scavenge the extents overflow file */

        if (vol.ext.hdr.bthFNode > 0)
        {
            if (HfsBTree.bt_getnode(n, vol.ext, vol.ext.hdr.bthFNode) == -1)
                return -1;

            n.rnum = 0;

            while (true)
            {
                while (n.rnum >= n.nd.ndNRecs && n.nd.ndFLink > 0)
                {
                    if (HfsBTree.bt_getnode(n, vol.ext, n.nd.ndFLink) == -1)
                        return -1;

                    n.rnum = 0;
                }

                if (n.rnum >= n.nd.ndNRecs && n.nd.ndFLink == 0)
                    break;

                ExtDataRec data = new ExtDataRec();
                int extRecOff = n.roff[n.rnum];
                HfsRecord.r_unpackextdata(n.data,
                                          extRecOff + recKeySkip(n.data, extRecOff),
                                          data);

                markexts(vbm, data);

                ++n.rnum;
            }
        }

        /* scavenge the catalog file */

        if (vol.cat.hdr.bthFNode > 0)
        {
            if (HfsBTree.bt_getnode(n, vol.cat, vol.cat.hdr.bthFNode) == -1)
                return -1;

            n.rnum = 0;

            while (true)
            {
                while (n.rnum >= n.nd.ndNRecs && n.nd.ndFLink > 0)
                {
                    if (HfsBTree.bt_getnode(n, vol.cat, n.nd.ndFLink) == -1)
                        return -1;

                    n.rnum = 0;
                }

                if (n.rnum >= n.nd.ndNRecs && n.nd.ndFLink == 0)
                    break;

                CatDataRec data = new CatDataRec();
                int catRecOff = n.roff[n.rnum];
                HfsRecord.r_unpackcatdata(n.data,
                                          catRecOff + recKeySkip(n.data, catRecOff),
                                          data);

                switch (data.cdrType)
                {
                case CatDataType.CDR_FIL_REC:
                    markexts(vbm, data.filExtRec);
                    markexts(vbm, data.filRExtRec);

                    if (data.filFlNum > lastcnid)
                        lastcnid = data.filFlNum;
                    break;

                case CatDataType.CDR_DIR_REC:
                    if (data.dirDirID > lastcnid)
                        lastcnid = data.dirDirID;
                    break;
                }

                ++n.rnum;
            }
        }

        /* count free blocks */

        int blks = 0;
        for (pt = vol.mdb.drNmAlBlks & 0xffff; pt-- > 0; )
        {
            if (! bmtst(vbm, pt))
                ++blks;
        }

        if ((vol.mdb.drFreeBks & 0xffff) != blks)
        {
            vol.mdb.drFreeBks = (short) blks;
            vol.flags |= HFS_VOL_UPDATE_MDB;
        }

        /* ensure next CNID is sane */

        if (vol.mdb.drNxtCNID <= lastcnid)
        {
            vol.mdb.drNxtCNID = lastcnid + 1;
            vol.flags |= HFS_VOL_UPDATE_MDB;
        }

        return 0;
    }
}
