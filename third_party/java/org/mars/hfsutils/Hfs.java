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
 * $Id: hfs.c,v 1.15 1998/11/02 22:09:00 rob Exp $
 */

package org.mars.hfsutils;

import org.mars.hfsutils.os.HfsOs;

import java.util.Arrays;

import static org.mars.hfsutils.HfsConstants.*;
import static org.mars.hfsutils.HfsException.*;

/**
 * High-level HFS volume API — faithful Java translation of
 * {@code dep/hfsutils/libhfs/hfs.c} and {@code hfs.h}.
 *
 * <p>Mount/umount, flush, getvol/setvol, vstat/vsetattr,
 * chdir/getcwd/setcwd/dirinfo, opendir/readdir/closedir,
 * create/open/setfork/getfork/read/write/truncate/seek/close,
 * stat/fstat/setattr/fsetattr, mkdir/rmdir, delete/rename,
 * zero/mkpart/nparts/format.
 *
 * <p>Functions that require I/O ({@code hfsMount}, {@code hfsFormat},
 * {@code hfsZero}, {@code hfsMkpart}, {@code hfsNparts}) accept an
 * {@link HfsOs} parameter — the Java equivalent of the C
 * {@code os_open} function table — because Java has no global
 * function pointers.
 *
 * <p>Public methods throw {@link HfsException} on error.
 */
public final class Hfs
{
    private Hfs()
    {
    }

    /* -----------------------------------------------------------------------
     * Static fields (globals from hfs.c)
     * ----------------------------------------------------------------------- */

    public static String hfsError = "no error";
    public static int hfsErrno = 0;

    /* linked list of mounted volumes */
    public static HfsVol hfsMounts;

    /* current volume */
    private static HfsVol curVol;

    /* -----------------------------------------------------------------------
     * Helper methods
     * ----------------------------------------------------------------------- */

    /*
     * Helper — set hfs_error/hfs_errno and return -1 (mirrors the C ERROR
     * macro's fail: label).
     */
    private static int fail(int errno, String msg)
    {
        Hfs.hfsError = msg;
        Hfs.hfsErrno = errno;
        return -1;
    }

    /*
     * NAME:	validvname()
     * DESCRIPTION:	return true if parameter is a valid volume name
     */
    private static boolean validvname(String name)
    {
        int len = name.length();

        if (len < 1)
        {
            fail(EINVAL, "volume name cannot be empty");
            return false;
        }
        else if (len > HFS_MAX_VLEN)
        {
            fail(ENAMETOOLONG,
                 "volume name can be at most " + HFS_MAX_VLEN + " chars");
            return false;
        }

        if (name.indexOf(':') >= 0)
        {
            fail(EINVAL, "volume name may not contain colons");
            return false;
        }

        return true;
    }

    /*
     * NAME:	getvol()
     * DESCRIPTION:	validate a volume reference
     *
     * In C this takes {@code hfsvol **vol} (in/out pointer).  In Java the
     * array element {@code vol[0]} is replaced if it was null (falling back
     * to {@code curVol}).
     */
    private static int getvol(HfsVol[] vol)
    {
        if (vol[0] == null)
        {
            if (curVol == null)
            {
                fail(EINVAL, "no volume is current");
                return -1;
            }

            vol[0] = curVol;
        }

        return 0;
    }

    /* =======================================================================
     * High-Level Volume Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->mount()
     * DESCRIPTION:	open an HFS volume; return volume descriptor or null (error)
     *
     * NOTE: in the C API the {@code path} is an opaque device identifier
     * passed to {@code os_open}.  In Java we accept an {@link HfsOs} instance
     * that the caller provides; {@code path} is forwarded to
     * {@link HfsOs#open(String, int)} and used for
     * {@link HfsOs#same(String)} comparison.
     */
    public static HfsVol hfsMount(HfsOs os, String path, int pnum, int mode)
        throws HfsException
    {
        HfsVol vol = null;

        /* see if the volume is already mounted */

        for (HfsVol check = hfsMounts; check != null; check = check.next)
        {
            if (check.pnum == pnum && HfsVolume.v_same(check, path) == 1)
            {
                /* verify compatible read/write mode */

                if (((check.flags & HFS_VOL_READONLY) != 0 &&
                     (mode & HFS_MODE_RDWR) == 0) ||
                    ((check.flags & HFS_VOL_READONLY) == 0 &&
                     (mode & (HFS_MODE_RDWR | HFS_MODE_ANY)) != 0))
                {
                    vol = check;
                    break;
                }
            }
        }

        if (vol != null)
        {
            ++vol.refs;
            curVol = vol;
            return vol;
        }

        vol = new HfsVol();

        HfsVolume.v_init(vol, mode);

        /* open the medium */

        vol.priv = os;

        int modeMask = mode & HFS_MODE_MASK;

        boolean opened = false;

        if (modeMask == HFS_MODE_RDWR || modeMask == HFS_MODE_ANY)
        {
            if (HfsVolume.v_open(vol, path, HFS_MODE_RDWR) != -1)
            {
                opened = true;
            }
            else if (modeMask == 0 /* HFS_MODE_RDWR */)
            {
                HfsVolume.v_close(vol);
                throw new HfsException(hfsErrno, hfsError);
            }
            else
            {
                /* HFS_MODE_ANY: fall through to RDONLY */

                vol.flags |= HFS_VOL_READONLY;

                if (HfsVolume.v_open(vol, path, HFS_MODE_RDONLY) == -1)
                {
                    HfsVolume.v_close(vol);
                    throw new HfsException(hfsErrno, hfsError);
                }
                opened = true;
            }
        }
        else /* HFS_MODE_RDONLY / default */
        {
            vol.flags |= HFS_VOL_READONLY;

            if (HfsVolume.v_open(vol, path, HFS_MODE_RDONLY) == -1)
            {
                HfsVolume.v_close(vol);
                throw new HfsException(hfsErrno, hfsError);
            }
            opened = true;
        }

        /* mount the volume */

        if (HfsVolume.v_geometry(vol, pnum) == -1 ||
            HfsVolume.v_mount(vol) == -1)
        {
            HfsVolume.v_close(vol);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* add to linked list of volumes */

        vol.prev = null;
        vol.next = hfsMounts;

        if (hfsMounts != null)
            hfsMounts.prev = vol;

        hfsMounts = vol;

        ++vol.refs;
        curVol = vol;

        return vol;
    }

    /*
     * NAME:	hfs->flush()
     * DESCRIPTION:	flush all pending changes to an HFS volume
     */
    public static void hfsFlush(HfsVol vol) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        for (HfsFileHandle file = vol.files; file != null; file = file.next)
        {
            if (HfsFile.f_flush(file) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }

        if (HfsVolume.v_flush(vol) == -1)
            throw new HfsException(hfsErrno, hfsError);
    }

    /*
     * NAME:	hfs->flushall()
     * DESCRIPTION:	flush all pending changes to all mounted HFS volumes
     */
    public static void hfsFlushall() throws HfsException
    {
        for (HfsVol vol = hfsMounts; vol != null; vol = vol.next)
            hfsFlush(vol);
    }

    /*
     * NAME:	hfs->umount()
     * DESCRIPTION:	close an HFS volume
     */
    public static void hfsUmount(HfsVol vol) throws HfsException
    {
        int result = 0;
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (--vol.refs != 0)
        {
            if (HfsVolume.v_flush(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);
            return;
        }

        /* close all open files and directories */

        while (vol.files != null)
        {
            if (hfsClose(vol.files) != 0)
                result = -1;
        }

        while (vol.dirs != null)
        {
            if (hfsClosedir(vol.dirs) != 0)
                result = -1;
        }

        /* close medium */

        if (HfsVolume.v_close(vol) == -1)
            result = -1;

        /* remove from linked list of volumes */

        if (vol.prev != null)
            vol.prev.next = vol.next;
        if (vol.next != null)
            vol.next.prev = vol.prev;

        if (vol == hfsMounts)
            hfsMounts = vol.next;
        if (vol == curVol)
            curVol = null;

        if (result == -1)
            throw new HfsException(hfsErrno, hfsError);
    }

    /*
     * NAME:	hfs->umountall()
     * DESCRIPTION:	unmount all mounted volumes
     */
    public static void hfsUmountall() throws HfsException
    {
        while (hfsMounts != null)
            hfsUmount(hfsMounts);
    }

    /*
     * NAME:	hfs->getvol()
     * DESCRIPTION:	return a pointer to a mounted volume
     */
    public static HfsVol hfsGetvol(String name)
    {
        if (name == null)
            return curVol;

        for (HfsVol vol = hfsMounts; vol != null; vol = vol.next)
        {
            if (HfsData.d_relstring(name.toCharArray(), vol.mdb.drVN) == 0)
                return vol;
        }

        return null;
    }

    /*
     * NAME:	hfs->setvol()
     * DESCRIPTION:	change the current volume
     */
    public static void hfsSetvol(HfsVol vol)
    {
        curVol = vol;
    }

    /*
     * NAME:	hfs->vstat()
     * DESCRIPTION:	return volume statistics
     */
    public static void hfsVstat(HfsVol vol, HfsVolEnt ent) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        for (int i = 0; i < vol.mdb.drVN.length && vol.mdb.drVN[i] != 0; ++i)
            ent.name[i] = vol.mdb.drVN[i];
        ent.name[Math.min(vol.mdb.drVN.length, HFS_MAX_VLEN)] = 0;

        ent.flags = (vol.flags & HFS_VOL_READONLY) != 0 ? HFS_ISLOCKED : 0;

        ent.totbytes  = (vol.mdb.drNmAlBlks & 0xffff)
                        * (vol.mdb.drAlBlkSiz & 0xffffffffL);
        ent.freebytes = (vol.mdb.drFreeBks & 0xffff)
                        * (vol.mdb.drAlBlkSiz & 0xffffffffL);

        ent.alblocksz = vol.mdb.drAlBlkSiz & 0xffffffffL;
        ent.clumpsz   = vol.mdb.drClpSiz & 0xffffffffL;

        ent.numfiles  = vol.mdb.drFilCnt & 0xffff;
        ent.numdirs   = vol.mdb.drDirCnt & 0xffff;

        ent.crdate    = HfsData.d_ltime(vol.mdb.drCrDate);
        ent.mddate    = HfsData.d_ltime(vol.mdb.drLsMod);
        ent.bkdate    = HfsData.d_ltime(vol.mdb.drVolBkUp);

        ent.blessed   = vol.mdb.drFndrInfo[0];
    }

    /*
     * NAME:	hfs->vsetattr()
     * DESCRIPTION:	change volume attributes
     */
    public static void hfsVsetattr(HfsVol vol, HfsVolEnt ent)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (ent.clumpsz % (vol.mdb.drAlBlkSiz & 0xffffffffL) != 0)
        {
            fail(EINVAL, "illegal clump size");
            throw new HfsException(hfsErrno, hfsError);
        }

        /* make sure "blessed" folder exists */

        if (ent.blessed != 0 &&
            HfsVolume.v_getthread(vol, ent.blessed, null, null,
                                  CatDataType.CDR_THD_REC) <= 0)
        {
            fail(EINVAL, "illegal blessed folder");
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        vol.mdb.drClpSiz      = (int) ent.clumpsz;

        vol.mdb.drCrDate      = HfsData.d_mtime(ent.crdate);
        vol.mdb.drLsMod       = HfsData.d_mtime(ent.mddate);
        vol.mdb.drVolBkUp     = HfsData.d_mtime(ent.bkdate);

        vol.mdb.drFndrInfo[0] = ent.blessed;

        vol.flags |= HFS_VOL_UPDATE_MDB;
    }

    /* =======================================================================
     * High-Level Directory Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->chdir()
     * DESCRIPTION:	change current HFS directory
     */
    public static void hfsChdir(HfsVol vol, String path) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatDataRec data = new CatDataRec();

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, path, data, null, null, null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (data.cdrType != CatDataType.CDR_DIR_REC)
        {
            fail(ENOTDIR, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        vol.cwd = data.dirDirID;
    }

    /*
     * NAME:	hfs->getcwd()
     * DESCRIPTION:	return the current working directory ID
     */
    public static long hfsGetcwd(HfsVol vol) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        return vol.cwd;
    }

    /*
     * NAME:	hfs->setcwd()
     * DESCRIPTION:	set the current working directory ID
     */
    public static void hfsSetcwd(HfsVol vol, long id) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (id == vol.cwd)
            return;

        /* make sure the directory exists */

        if (HfsVolume.v_getthread(vol, id, null, null,
                                  CatDataType.CDR_THD_REC) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol.cwd = id;
    }

    /*
     * NAME:	hfs->dirinfo()
     * DESCRIPTION:	given a directory ID, return its (name and) parent ID
     *
     * {@code id[0]} is an in/out parameter: on entry the directory CNID,
     * on exit the parent CNID.
     */
    public static void hfsDirinfo(HfsVol vol, long[] id, char[] name)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatDataRec thread = new CatDataRec();

        if (getvol(volArr) == -1 ||
            HfsVolume.v_getthread(volArr[0], id[0], thread, null,
                                  CatDataType.CDR_THD_REC) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        id[0] = thread.thdParID;

        if (name != null)
        {
            int len = 0;
            while (len < thread.thdCName.length && thread.thdCName[len] != 0)
                ++len;
            System.arraycopy(thread.thdCName, 0, name, 0, len);
            name[len] = 0;
        }
    }

    /*
     * NAME:	hfs->opendir()
     * DESCRIPTION:	prepare to read the contents of a directory
     */
    public static HfsDir hfsOpendir(HfsVol vol, String path)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        HfsDir dir;
        CatKeyRec key = new CatKeyRec();
        CatDataRec data = new CatDataRec();
        byte[] pkey = new byte[HFS_CATKEYLEN];

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        dir = new HfsDir();

        dir.vol = vol;

        if (path.isEmpty())
        {
            /* meta-directory containing root dirs from all mounted volumes */

            dir.dirid = 0;
            dir.vptr  = hfsMounts;
        }
        else
        {
            if (HfsVolume.v_resolve(volArr, path, data, null, null, null) <= 0)
                throw new HfsException(hfsErrno, hfsError);

            vol = volArr[0];

            if (data.cdrType != CatDataType.CDR_DIR_REC)
            {
                fail(ENOTDIR, null);
                throw new HfsException(hfsErrno, hfsError);
            }

            dir.dirid = data.dirDirID;
            dir.vptr  = null;

            HfsRecord.r_makecatkey(key, dir.dirid, "");
            HfsRecord.r_packcatkey(key, pkey, null);

            if (HfsBTree.bt_search(vol.cat, pkey, dir.n) <= 0)
                throw new HfsException(hfsErrno, hfsError);
        }

        dir.prev = null;
        dir.next = vol.dirs;

        if (vol.dirs != null)
            vol.dirs.prev = dir;

        vol.dirs = dir;

        return dir;
    }

    /*
     * NAME:	hfs->readdir()
     * DESCRIPTION:	return the next entry in the directory
     */
    public static void hfsReaddir(HfsDir dir, HfsDirEnt ent) throws HfsException
    {
        CatKeyRec key = new CatKeyRec();
        CatDataRec data = new CatDataRec();

        if (dir.dirid == 0)
        {
            /* meta-directory: iterate over root dirs of all mounted volumes */

            HfsVol vol;

            for (vol = hfsMounts; vol != null; vol = vol.next)
            {
                if (vol == dir.vptr)
                    break;
            }

            if (vol == null)
            {
                fail(ENOENT, "no more entries");
                throw new HfsException(hfsErrno, hfsError);
            }

            if (HfsVolume.v_getthread(vol, HFS_CNID_ROOTDIR, data, null,
                                      CatDataType.CDR_THD_REC) <= 0 ||
                HfsVolume.v_catsearch(vol, HFS_CNID_ROOTPAR,
                                      new String(data.thdCName),
                                      data, null, null) <= 0)
                throw new HfsException(hfsErrno, hfsError);

            HfsRecord.r_unpackdirent(HFS_CNID_ROOTPAR,
                                     new String(data.thdCName), data, ent);

            dir.vptr = vol.next;

            return;
        }

        if (dir.n.rnum == -1)
        {
            fail(ENOENT, "no more entries");
            throw new HfsException(hfsErrno, hfsError);
        }

        while (true)
        {
            ++dir.n.rnum;

            while (dir.n.rnum >= dir.n.nd.ndNRecs)
            {
                if (dir.n.nd.ndFLink == 0)
                {
                    dir.n.rnum = -1;
                    fail(ENOENT, "no more entries");
                    throw new HfsException(hfsErrno, hfsError);
                }

                if (HfsBTree.bt_getnode(dir.n, dir.n.bt, dir.n.nd.ndFLink) == -1)
                {
                    dir.n.rnum = -1;
                    throw new HfsException(hfsErrno, hfsError);
                }

                dir.n.rnum = 0;
            }

            int recOff = dir.n.roff[dir.n.rnum];

            /* unpack key from the record at recOff */

            HfsRecord.r_unpackcatkey(dir.n.data, recOff, key);

            int keyLen = key.ckrKeyLen & 0xff;
            int keySkip = ((1 + keyLen + 1) & ~1);

            if (key.ckrParID != dir.dirid)
            {
                dir.n.rnum = -1;
                fail(ENOENT, "no more entries");
                throw new HfsException(hfsErrno, hfsError);
            }

            /* unpack data portion (after the key) */

            int dataOff = recOff + keySkip;

            HfsRecord.r_unpackcatdata(dir.n.data, dataOff, data);

            switch (data.cdrType)
            {
            case CatDataType.CDR_DIR_REC:
            case CatDataType.CDR_FIL_REC:
                HfsRecord.r_unpackdirent(key.ckrParID,
                                         new String(key.ckrCName), data, ent);
                return;

            case CatDataType.CDR_THD_REC:
            case CatDataType.CDR_FTHD_REC:
                break;

            default:
                dir.n.rnum = -1;
                fail(EIO, "unexpected directory entry found");
                throw new HfsException(hfsErrno, hfsError);
            }
        }
    }

    /*
     * NAME:	hfs->closedir()
     * DESCRIPTION:	stop reading a directory
     */
    public static int hfsClosedir(HfsDir dir)
    {
        HfsVol vol = dir.vol;

        if (dir.prev != null)
            dir.prev.next = dir.next;
        if (dir.next != null)
            dir.next.prev = dir.prev;
        if (dir == vol.dirs)
            vol.dirs = dir.next;

        return 0;
    }

    /* =======================================================================
     * High-Level File Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->create()
     * DESCRIPTION:	create and open a new file
     */
    public static HfsFileHandle hfsCreate(HfsVol vol, String path,
                                          String type, String creator)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        HfsFileHandle file;
        long[] paridArr = new long[1];
        char[] name = new char[HFS_MAX_FLEN + 1];
        CatKeyRec key = new CatKeyRec();
        byte[] record = new byte[HFS_MAX_CATRECLEN];
        int[] reclen = new int[1];
        int found;

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        file = new HfsFileHandle();

        found = HfsVolume.v_resolve(volArr, path, file.cat, paridArr, name, null);
        vol = volArr[0];
        if (found == -1 || paridArr[0] == 0)
            throw new HfsException(hfsErrno, hfsError);

        if (found != 0)
        {
            fail(EEXIST, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if (paridArr[0] == HFS_CNID_ROOTPAR)
        {
            fail(EINVAL, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* create file `name' in parent `parid' */

        if (HfsBTree.bt_space(vol.cat, 1) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* convert name char[] to String for f_init */
        int nameLen = 0;
        while (nameLen < name.length && name[nameLen] != 0)
            ++nameLen;
        String nameStr = new String(name, 0, nameLen);

        HfsFile.f_init(file, vol, vol.mdb.drNxtCNID++, nameStr);
        vol.flags |= HFS_VOL_UPDATE_MDB;

        file.parid = paridArr[0];

        /* create catalog record */

        byte[] typeBytes = new byte[4];
        byte[] creatorBytes = new byte[4];
        for (int i = 0; i < 4; ++i)
        {
            typeBytes[i]    = (i < type.length())    ? (byte) type.charAt(i)    : 0;
            creatorBytes[i] = (i < creator.length()) ? (byte) creator.charAt(i) : 0;
        }

        file.cat.filUsrWds.fdType    = HfsData.d_getsl(typeBytes, 0);
        file.cat.filUsrWds.fdCreator = HfsData.d_getsl(creatorBytes, 0);

        file.cat.filCrDat = HfsData.d_mtime(System.currentTimeMillis() / 1000);
        file.cat.filMdDat = file.cat.filCrDat;

        HfsRecord.r_makecatkey(key, file.parid, new String(file.name));
        HfsRecord.r_packcatrec(key, file.cat, record, reclen);

        if (HfsBTree.bt_insert(vol.cat, record, reclen[0]) == -1 ||
            HfsVolume.v_adjvalence(vol, file.parid, false, 1) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* package file handle for user */

        file.next = vol.files;

        if (vol.files != null)
            vol.files.prev = file;

        vol.files = file;

        return file;
    }

    /*
     * NAME:	hfs->open()
     * DESCRIPTION:	prepare a file for I/O
     */
    public static HfsFileHandle hfsOpen(HfsVol vol, String path)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        HfsFileHandle file;

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        file = new HfsFileHandle();

        if (HfsVolume.v_resolve(volArr, path, file.cat, new long[]{ 0 },
                                file.name, null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (file.cat.cdrType != CatDataType.CDR_FIL_REC)
        {
            fail(EISDIR, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* package file handle for user */

        file.vol   = vol;
        file.flags = 0;

        HfsFile.f_selectfork(file, FK_DATA);

        file.prev = null;
        file.next = vol.files;

        if (vol.files != null)
            vol.files.prev = file;

        vol.files = file;

        return file;
    }

    /*
     * NAME:	hfs->setfork()
     * DESCRIPTION:	select file fork for I/O operations
     */
    public static void hfsSetfork(HfsFileHandle file, int fork)
        throws HfsException
    {
        if (HfsFile.f_trunc(file) == -1)
            throw new HfsException(hfsErrno, hfsError);

        HfsFile.f_selectfork(file, fork != 0 ? FK_RSRC : FK_DATA);
    }

    /*
     * NAME:	hfs->getfork()
     * DESCRIPTION:	return the current fork for I/O operations
     */
    public static int hfsGetfork(HfsFileHandle file)
    {
        return file.fork != FK_DATA ? 1 : 0;
    }

    /*
     * NAME:	hfs->read()
     * DESCRIPTION:	read from an open file
     */
    public static long hfsRead(HfsFileHandle file, byte[] buf, long len)
        throws HfsException
    {
        long[] lglen = new long[1];

        HfsFile.f_getptrs(file, null, lglen, null);

        if (file.pos + len > lglen[0])
            len = lglen[0] - file.pos;

        long count = len;
        int bufOffset = 0;

        while (count > 0)
        {
            long bnum, offs, chunk;

            bnum  = file.pos >> HFS_BLOCKSZ_BITS;
            offs  = file.pos & (HFS_BLOCKSZ - 1);

            chunk = HFS_BLOCKSZ - offs;
            if (chunk > count)
                chunk = count;

            byte[] b = new byte[HFS_BLOCKSZ];

            if (HfsFile.f_getblock(file, bnum, b) == -1)
                throw new HfsException(hfsErrno, hfsError);

            System.arraycopy(b, (int) offs, buf, bufOffset, (int) chunk);

            bufOffset += (int) chunk;

            file.pos += chunk;
            count    -= chunk;
        }

        return len;
    }

    /*
     * NAME:	hfs->write()
     * DESCRIPTION:	write to an open file
     */
    public static long hfsWrite(HfsFileHandle file, byte[] buf, long len)
        throws HfsException
    {
        long[] lglen = new long[1];
        long[] pylen = new long[1];
        long count;

        if ((file.vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        HfsFile.f_getptrs(file, null, lglen, pylen);

        count = len;

        /* set flag to update (at least) the modification time */

        if (count > 0)
        {
            file.cat.filMdDat = HfsData.d_mtime(System.currentTimeMillis() / 1000);
            file.flags |= HFS_FILE_UPDATE_CATREC;
        }

        int bufOffset = 0;

        while (count > 0)
        {
            long bnum, offs, chunk;

            bnum  = file.pos >> HFS_BLOCKSZ_BITS;
            offs  = file.pos & (HFS_BLOCKSZ - 1);

            chunk = HFS_BLOCKSZ - offs;
            if (chunk > count)
                chunk = count;

            if (file.pos + chunk > pylen[0])
            {
                if (HfsBTree.bt_space(file.vol.ext, 1) == -1 ||
                    HfsFile.f_alloc(file) == -1)
                    throw new HfsException(hfsErrno, hfsError);

                pylen[0] = file.cat.filPyLen;
            }

            byte[] b = new byte[HFS_BLOCKSZ];

            if (offs != 0 || chunk != HFS_BLOCKSZ)
            {
                if (HfsFile.f_getblock(file, bnum, b) == -1)
                    throw new HfsException(hfsErrno, hfsError);
            }

            System.arraycopy(buf, bufOffset, b, (int) offs, (int) chunk);

            if (HfsFile.f_putblock(file, bnum, b) == -1)
                throw new HfsException(hfsErrno, hfsError);

            bufOffset += (int) chunk;

            file.pos += chunk;
            count    -= chunk;

            if (file.pos > lglen[0])
                lglen[0] = file.pos;
        }

        file.cat.filLgLen = lglen[0];

        return len;
    }

    /*
     * NAME:	hfs->truncate()
     * DESCRIPTION:	truncate an open file
     */
    public static void hfsTruncate(HfsFileHandle file, long len)
        throws HfsException
    {
        long[] lglen = new long[1];

        HfsFile.f_getptrs(file, null, lglen, null);

        if (lglen[0] > len)
        {
            if ((file.vol.flags & HFS_VOL_READONLY) != 0)
            {
                fail(EROFS, null);
                throw new HfsException(hfsErrno, hfsError);
            }

            lglen[0] = len;

            file.cat.filMdDat = HfsData.d_mtime(System.currentTimeMillis() / 1000);
            file.flags |= HFS_FILE_UPDATE_CATREC;

            if (file.pos > len)
                file.pos = len;
        }
    }

    /*
     * NAME:	hfs->seek()
     * DESCRIPTION:	change file seek pointer
     */
    public static long hfsSeek(HfsFileHandle file, long offset, int from)
        throws HfsException
    {
        long[] lglen = new long[1];
        long newpos;

        HfsFile.f_getptrs(file, null, lglen, null);

        if (from == HFS_SEEK_SET)
        {
            newpos = (offset < 0) ? 0 : offset;
        }
        else if (from == HFS_SEEK_CUR)
        {
            if (offset < 0 && (long) -offset > file.pos)
                newpos = 0;
            else
                newpos = file.pos + offset;
        }
        else if (from == HFS_SEEK_END)
        {
            if (offset < 0 && (long) -offset > lglen[0])
                newpos = 0;
            else
                newpos = lglen[0] + offset;
        }
        else
        {
            fail(EINVAL, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if (newpos > lglen[0])
            newpos = lglen[0];

        file.pos = newpos;

        return newpos;
    }

    /*
     * NAME:	hfs->close()
     * DESCRIPTION:	close a file
     */
    public static int hfsClose(HfsFileHandle file)
    {
        HfsVol vol = file.vol;
        int result = 0;

        if (HfsFile.f_trunc(file) == -1 ||
            HfsFile.f_flush(file) == -1)
            result = -1;

        if (file.prev != null)
            file.prev.next = file.next;
        if (file.next != null)
            file.next.prev = file.prev;
        if (file == vol.files)
            vol.files = file.next;

        return result;
    }

    /* =======================================================================
     * High-Level Catalog Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->stat()
     * DESCRIPTION:	return catalog information for an arbitrary path
     */
    public static void hfsStat(HfsVol vol, String path, HfsDirEnt ent)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatDataRec data = new CatDataRec();
        long[] paridArr = new long[1];
        char[] name = new char[HFS_MAX_FLEN + 1];

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, path, data, paridArr, name, null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        HfsRecord.r_unpackdirent(paridArr[0], new String(name), data, ent);
    }

    /*
     * NAME:	hfs->fstat()
     * DESCRIPTION:	return catalog information for an open file
     */
    public static void hfsFstat(HfsFileHandle file, HfsDirEnt ent)
    {
        HfsRecord.r_unpackdirent(file.parid, new String(file.name),
                                 file.cat, ent);
    }

    /*
     * NAME:	hfs->setattr()
     * DESCRIPTION:	change a file's attributes
     */
    public static void hfsSetattr(HfsVol vol, String path, HfsDirEnt ent)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatDataRec data = new CatDataRec();
        Node n = new Node();

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, path, data, null, null, n) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        HfsRecord.r_packdirent(data, ent);

        if (HfsVolume.v_putcatrec(data, n) == -1)
            throw new HfsException(hfsErrno, hfsError);
    }

    /*
     * NAME:	hfs->fsetattr()
     * DESCRIPTION:	change an open file's attributes
     */
    public static void hfsFsetattr(HfsFileHandle file, HfsDirEnt ent)
        throws HfsException
    {
        if ((file.vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        HfsRecord.r_packdirent(file.cat, ent);

        file.flags |= HFS_FILE_UPDATE_CATREC;
    }

    /* =======================================================================
     * High-Level Directory Manipulation Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->mkdir()
     * DESCRIPTION:	create a new directory
     */
    public static void hfsMkdir(HfsVol vol, String path) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatDataRec data = new CatDataRec();
        long[] paridArr = new long[1];
        char[] name = new char[HFS_MAX_FLEN + 1];
        int found;

        if (getvol(volArr) == -1)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        found = HfsVolume.v_resolve(volArr, path, data, paridArr, name, null);
        vol = volArr[0];
        if (found == -1 || paridArr[0] == 0)
            throw new HfsException(hfsErrno, hfsError);

        if (found != 0)
        {
            fail(EEXIST, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if (paridArr[0] == HFS_CNID_ROOTPAR)
        {
            fail(EINVAL, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        int nameLen = 0;
        while (nameLen < name.length && name[nameLen] != 0)
            ++nameLen;

        if (HfsVolume.v_mkdir(vol, paridArr[0], new String(name, 0, nameLen)) == -1)
            throw new HfsException(hfsErrno, hfsError);
    }

    /*
     * NAME:	hfs->rmdir()
     * DESCRIPTION:	delete an empty directory
     */
    public static void hfsRmdir(HfsVol vol, String path) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        CatKeyRec key = new CatKeyRec();
        CatDataRec data = new CatDataRec();
        long[] paridArr = new long[1];
        char[] name = new char[HFS_MAX_FLEN + 1];
        byte[] pkey = new byte[HFS_CATKEYLEN];

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, path, data, paridArr, name, null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (data.cdrType != CatDataType.CDR_DIR_REC)
        {
            fail(ENOTDIR, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((data.dirVal & 0xffff) != 0)
        {
            fail(ENOTEMPTY, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if (paridArr[0] == HFS_CNID_ROOTPAR)
        {
            fail(EINVAL, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* delete directory record */

        int nameLen = 0;
        while (nameLen < name.length && name[nameLen] != 0)
            ++nameLen;
        String nameStr = new String(name, 0, nameLen);

        HfsRecord.r_makecatkey(key, paridArr[0], nameStr);
        HfsRecord.r_packcatkey(key, pkey, null);

        if (HfsBTree.bt_delete(vol.cat, pkey) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* delete thread record */

        HfsRecord.r_makecatkey(key, data.dirDirID, "");
        HfsRecord.r_packcatkey(key, pkey, null);

        if (HfsBTree.bt_delete(vol.cat, pkey) == -1 ||
            HfsVolume.v_adjvalence(vol, paridArr[0], true, -1) == -1)
            throw new HfsException(hfsErrno, hfsError);
    }

    /* =======================================================================
     * High-Level File Manipulation Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->delete()
     * DESCRIPTION:	remove both forks of a file
     */
    public static void hfsDelete(HfsVol vol, String path) throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        HfsFileHandle file = new HfsFileHandle();
        CatKeyRec key = new CatKeyRec();
        byte[] pkey = new byte[HFS_CATKEYLEN];
        int found;

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, path, file.cat, new long[]{ 0 },
                                file.name, null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        if (file.cat.cdrType != CatDataType.CDR_FIL_REC)
        {
            fail(EISDIR, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if (file.parid == HFS_CNID_ROOTPAR)
        {
            fail(EINVAL, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* free allocation blocks */

        file.vol   = vol;
        file.flags = 0;

        file.cat.filLgLen  = 0;
        file.cat.filRLgLen = 0;

        HfsFile.f_selectfork(file, FK_DATA);
        if (HfsFile.f_trunc(file) == -1)
            throw new HfsException(hfsErrno, hfsError);

        HfsFile.f_selectfork(file, FK_RSRC);
        if (HfsFile.f_trunc(file) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* delete file record */

        String nameStr = new String(file.name);

        HfsRecord.r_makecatkey(key, file.parid, nameStr);
        HfsRecord.r_packcatkey(key, pkey, null);

        if (HfsBTree.bt_delete(vol.cat, pkey) == -1 ||
            HfsVolume.v_adjvalence(vol, file.parid, false, -1) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* delete file thread, if any */

        found = HfsVolume.v_getthread(vol, file.cat.filFlNum, null, null,
                                      CatDataType.CDR_FTHD_REC);
        if (found == -1)
            throw new HfsException(hfsErrno, hfsError);

        if (found != 0)
        {
            HfsRecord.r_makecatkey(key, file.cat.filFlNum, "");
            HfsRecord.r_packcatkey(key, pkey, null);

            if (HfsBTree.bt_delete(vol.cat, pkey) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
    }

    /*
     * NAME:	hfs->rename()
     * DESCRIPTION:	change the name of and/or move a file or directory
     */
    public static void hfsRename(HfsVol vol, String srcpath, String dstpath)
        throws HfsException
    {
        HfsVol[] volArr = new HfsVol[]{ vol };
        HfsVol srcvol;
        CatDataRec src = new CatDataRec();
        CatDataRec dst = new CatDataRec();
        long[] srcidArr = new long[1];
        long[] dstidArr = new long[1];
        CatKeyRec key = new CatKeyRec();
        char[] srcname = new char[HFS_MAX_FLEN + 1];
        char[] dstname = new char[HFS_MAX_FLEN + 1];
        byte[] record = new byte[HFS_MAX_CATRECLEN];
        int[] reclen = new int[1];
        int found, isdir, moving;
        Node n = new Node();

        if (getvol(volArr) == -1 ||
            HfsVolume.v_resolve(volArr, srcpath, src, srcidArr, srcname,
                                null) <= 0)
            throw new HfsException(hfsErrno, hfsError);

        vol = volArr[0];

        isdir  = (src.cdrType == CatDataType.CDR_DIR_REC) ? 1 : 0;
        srcvol = vol;

        found = HfsVolume.v_resolve(volArr, dstpath, dst, dstidArr, dstname, null);
        vol = volArr[0];
        if (found == -1)
            throw new HfsException(hfsErrno, hfsError);

        if (vol != srcvol)
        {
            fail(EINVAL, "can't move across volumes");
            throw new HfsException(hfsErrno, hfsError);
        }

        if (dstidArr[0] == 0)
        {
            fail(ENOENT, "bad destination path");
            throw new HfsException(hfsErrno, hfsError);
        }

        if (found != 0 &&
            dst.cdrType == CatDataType.CDR_DIR_REC &&
            dst.dirDirID != src.dirDirID)
        {
            dstidArr[0] = dst.dirDirID;

            int srcLen = 0;
            while (srcLen < srcname.length && srcname[srcLen] != 0)
                ++srcLen;
            System.arraycopy(srcname, 0, dstname, 0, srcLen);
            dstname[srcLen] = 0;

            found = HfsVolume.v_catsearch(vol, dstidArr[0],
                                          new String(dstname, 0, srcLen),
                                          null, null, null);
            if (found == -1)
                throw new HfsException(hfsErrno, hfsError);
        }

        moving = (srcidArr[0] != dstidArr[0]) ? 1 : 0;

        if (found != 0)
        {
            int ptr;

            ptr = dstpath.lastIndexOf(':');
            if (ptr < 0)
                ptr = 0;
            else
                ++ptr;

            if (ptr < dstpath.length() && dstpath.charAt(ptr) != '\0')
            {
                int copyLen = dstpath.length() - ptr;
                for (int i = 0; i < copyLen; ++i)
                    dstname[i] = dstpath.charAt(ptr + i);
                dstname[copyLen] = 0;
            }

            int srcLen = 0;
            while (srcLen < srcname.length && srcname[srcLen] != 0)
                ++srcLen;
            int dstLen = 0;
            while (dstLen < dstname.length && dstname[dstLen] != 0)
                ++dstLen;

            if (moving == 0 && srcLen == dstLen)
            {
                boolean eq = true;
                for (int i = 0; i < srcLen; ++i)
                {
                    if (srcname[i] != dstname[i])
                    {
                        eq = false;
                        break;
                    }
                }
                if (eq)
                    return;  /* source and destination are identical */
            }

            if (moving != 0 ||
                HfsData.d_relstring(srcname, dstname) != 0)
            {
                fail(EEXIST, "can't use destination name");
                throw new HfsException(hfsErrno, hfsError);
            }
        }

        /* can't move anything into the root directory's parent */

        if (moving != 0 && dstidArr[0] == HFS_CNID_ROOTPAR)
        {
            fail(EINVAL, "can't move above root directory");
            throw new HfsException(hfsErrno, hfsError);
        }

        if (moving != 0 && isdir != 0)
        {
            /* can't move root directory anywhere */

            if (src.dirDirID == HFS_CNID_ROOTDIR)
            {
                fail(EINVAL, "can't move root directory");
                throw new HfsException(hfsErrno, hfsError);
            }

            /* make sure we aren't trying to move a directory inside itself */

            long id;
            for (id = dstidArr[0]; id != HFS_CNID_ROOTDIR;
                 id = dst.thdParID)
            {
                if (id == src.dirDirID)
                {
                    fail(EINVAL, "can't move directory inside itself");
                    throw new HfsException(hfsErrno, hfsError);
                }

                if (HfsVolume.v_getthread(vol, id, dst, null,
                                          CatDataType.CDR_THD_REC) <= 0)
                    throw new HfsException(hfsErrno, hfsError);
            }
        }

        if ((vol.flags & HFS_VOL_READONLY) != 0)
        {
            fail(EROFS, null);
            throw new HfsException(hfsErrno, hfsError);
        }

        /* change volume name */

        if (dstidArr[0] == HFS_CNID_ROOTPAR)
        {
            int dstLen = 0;
            while (dstLen < dstname.length && dstname[dstLen] != 0)
                ++dstLen;
            String dstNameStr = new String(dstname, 0, dstLen);

            if (!validvname(dstNameStr))
                throw new HfsException(hfsErrno, hfsError);

            for (int i = 0; i <= dstLen; ++i)
                vol.mdb.drVN[i] = dstname[i];

            vol.flags |= HFS_VOL_UPDATE_MDB;
        }

        /* remove source record */

        int srcLen = 0;
        while (srcLen < srcname.length && srcname[srcLen] != 0)
            ++srcLen;
        String srcNameStr = new String(srcname, 0, srcLen);

        HfsRecord.r_makecatkey(key, srcidArr[0], srcNameStr);
        HfsRecord.r_packcatkey(key, record, null);

        if (HfsBTree.bt_delete(vol.cat, record) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* insert destination record */

        int dstLen = 0;
        while (dstLen < dstname.length && dstname[dstLen] != 0)
            ++dstLen;
        String dstNameStr = new String(dstname, 0, dstLen);

        HfsRecord.r_makecatkey(key, dstidArr[0], dstNameStr);
        HfsRecord.r_packcatrec(key, src, record, reclen);

        if (HfsBTree.bt_insert(vol.cat, record, reclen[0]) == -1)
            throw new HfsException(hfsErrno, hfsError);

        /* update thread record */

        if (isdir != 0)
        {
            if (HfsVolume.v_getthread(vol, src.dirDirID, dst, n,
                                      CatDataType.CDR_THD_REC) <= 0)
                throw new HfsException(hfsErrno, hfsError);

            dst.thdParID = dstidArr[0];
            for (int i = 0; i <= dstLen; ++i)
                dst.thdCName[i] = dstname[i];

            if (HfsVolume.v_putcatrec(dst, n) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
        else
        {
            found = HfsVolume.v_getthread(vol, src.filFlNum, dst, n,
                                          CatDataType.CDR_FTHD_REC);
            if (found == -1)
                throw new HfsException(hfsErrno, hfsError);

            if (found != 0)
            {
                dst.fthdParID = dstidArr[0];
                for (int i = 0; i <= dstLen; ++i)
                    dst.fthdCName[i] = dstname[i];

                if (HfsVolume.v_putcatrec(dst, n) == -1)
                    throw new HfsException(hfsErrno, hfsError);
            }
        }

        /* update directory valences */

        if (moving != 0)
        {
            if (HfsVolume.v_adjvalence(vol, srcidArr[0], isdir != 0, -1) == -1 ||
                HfsVolume.v_adjvalence(vol, dstidArr[0], isdir != 0,  1) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
    }

    /* =======================================================================
     * High-Level Media Routines
     * ======================================================================= */

    /*
     * NAME:	hfs->zero()
     * DESCRIPTION:	initialize medium with new/empty DDR and partition map
     */
    public static void hfsZero(HfsOs os, String path, int maxparts,
                               long[] blocks) throws HfsException
    {
        HfsVol vol = new HfsVol();

        HfsVolume.v_init(vol, HFS_OPT_NOCACHE);

        if (maxparts < 1)
        {
            fail(EINVAL, "must allow at least 1 partition");
            throw new HfsException(hfsErrno, hfsError);
        }

        vol.priv = os;

        try
        {
            if (HfsVolume.v_open(vol, path, HFS_MODE_RDWR) == -1 ||
                HfsVolume.v_geometry(vol, 0) == -1)
                throw new HfsException(hfsErrno, hfsError);

            if (HfsMedium.m_zeroddr(vol) == -1 ||
                HfsMedium.m_zeropm(vol, 1 + maxparts) == -1)
                throw new HfsException(hfsErrno, hfsError);

            if (blocks != null)
            {
                Partition map = new Partition();
                long[] bnumArr = new long[]{ 0 };
                int found;

                found = HfsMedium.m_findpmentry(vol, "Apple_Free", map, bnumArr);
                if (found == -1)
                    throw new HfsException(hfsErrno, hfsError);

                if (found == 0)
                {
                    fail(EIO, "unable to determine free partition space");
                    throw new HfsException(hfsErrno, hfsError);
                }

                blocks[0] = map.pmPartBlkCnt & 0xffffffffL;
            }

            if (HfsVolume.v_close(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
        catch (HfsException e)
        {
            HfsVolume.v_close(vol);
            throw e;
        }
    }

    /*
     * NAME:	hfs->mkpart()
     * DESCRIPTION:	create a new HFS partition
     */
    public static void hfsMkpart(HfsOs os, String path, long len)
        throws HfsException
    {
        HfsVol vol = new HfsVol();

        HfsVolume.v_init(vol, HFS_OPT_NOCACHE);

        vol.priv = os;

        try
        {
            if (HfsVolume.v_open(vol, path, HFS_MODE_RDWR) == -1)
                throw new HfsException(hfsErrno, hfsError);

            if (HfsMedium.m_mkpart(vol, "MacOS", "Apple_HFS", len) == -1)
                throw new HfsException(hfsErrno, hfsError);

            if (HfsVolume.v_close(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
        catch (HfsException e)
        {
            HfsVolume.v_close(vol);
            throw e;
        }
    }

    /*
     * NAME:	hfs->nparts()
     * DESCRIPTION:	return the number of HFS partitions in the medium
     */
    public static int hfsNparts(HfsOs os, String path) throws HfsException
    {
        HfsVol vol = new HfsVol();
        int nparts;
        Partition map = new Partition();
        long[] bnumArr = new long[]{ 0 };

        HfsVolume.v_init(vol, HFS_OPT_NOCACHE);

        vol.priv = os;

        try
        {
            if (HfsVolume.v_open(vol, path, HFS_MODE_RDONLY) == -1)
                throw new HfsException(hfsErrno, hfsError);

            nparts = 0;
            while (true)
            {
                int found = HfsMedium.m_findpmentry(vol, "Apple_HFS", map, bnumArr);
                if (found == -1)
                    throw new HfsException(hfsErrno, hfsError);

                if (found == 0)
                    break;

                ++nparts;
            }

            if (HfsVolume.v_close(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);

            return nparts;
        }
        catch (HfsException e)
        {
            HfsVolume.v_close(vol);
            throw e;
        }
    }

    /*
     * NAME:	compare()
     * DESCRIPTION:	comparison function for sorting blocks to be spared
     */
    private static int compare(Integer n1, Integer n2)
    {
        return n1 - n2;
    }

    /*
     * NAME:	hfs->format()
     * DESCRIPTION:	write a new filesystem
     */
    public static void hfsFormat(HfsOs os, String path, int pnum, int mode,
                                 String vname, int nbadblocks,
                                 long[] badblocks) throws HfsException
    {
        HfsVol vol = new HfsVol();
        BTree ext = vol.ext;
        BTree cat = vol.cat;
        int[] badalloc = null;

        HfsVolume.v_init(vol, mode);

        if (!validvname(vname))
            throw new HfsException(hfsErrno, hfsError);

        vol.priv = os;

        try
        {
            if (HfsVolume.v_open(vol, path, HFS_MODE_RDWR) == -1 ||
                HfsVolume.v_geometry(vol, pnum) == -1)
                throw new HfsException(hfsErrno, hfsError);

            /* initialize volume geometry */

            vol.lpa = 1 + (int) ((vol.vlen - 6) >> 16);

            if ((vol.flags & HFS_OPT_2048) != 0)
                vol.lpa = (vol.lpa + 3) & ~3;

            vol.vbmsz = (short) ((int) ((vol.vlen / vol.lpa + 0x0fff) >> 12));

            vol.mdb.drSigWord  = HFS_SIGWORD;
            vol.mdb.drCrDate   = HfsData.d_mtime(System.currentTimeMillis() / 1000);
            vol.mdb.drLsMod    = vol.mdb.drCrDate;
            vol.mdb.drAtrb     = 0;
            vol.mdb.drNmFls    = 0;
            vol.mdb.drVBMSt    = 3;
            vol.mdb.drAllocPtr = 0;

            vol.mdb.drAlBlkSiz = (int) (vol.lpa << HFS_BLOCKSZ_BITS);
            vol.mdb.drClpSiz   = vol.mdb.drAlBlkSiz << 2;
            vol.mdb.drAlBlSt   = (short) (vol.mdb.drVBMSt + (vol.vbmsz & 0xffff));

            if ((vol.flags & HFS_OPT_2048) != 0)
                vol.mdb.drAlBlSt =
                    (short) (((int) (vol.vstart & 3) + (vol.mdb.drAlBlSt & 0xffff)
                              + 3) & ~3);

            vol.mdb.drNmAlBlks =
                (short) ((int) ((vol.vlen - 2 - (vol.mdb.drAlBlSt & 0xffff))
                                / vol.lpa));

            vol.mdb.drNxtCNID  = HFS_CNID_ROOTDIR;  /* modified later */
            vol.mdb.drFreeBks  = vol.mdb.drNmAlBlks;

            for (int i = 0; i < vname.length() && i < 27; ++i)
                vol.mdb.drVN[i] = vname.charAt(i);
            vol.mdb.drVN[Math.min(vname.length(), 27)] = 0;

            vol.mdb.drVolBkUp  = 0;
            vol.mdb.drVSeqNum  = 0;
            vol.mdb.drWrCnt    = 0;

            vol.mdb.drXTClpSiz = (vol.mdb.drNmAlBlks & 0xffff) / 128
                                  * (vol.mdb.drAlBlkSiz & 0xffffffffL);
            vol.mdb.drCTClpSiz = vol.mdb.drXTClpSiz;

            vol.mdb.drNmRtDirs = 0;
            vol.mdb.drFilCnt   = 0;
            vol.mdb.drDirCnt   = (short) 0xffff;  /* -1 unsigned; incremented when root dir created */

            for (int i = 0; i < 8; ++i)
                vol.mdb.drFndrInfo[i] = 0;

            vol.mdb.drEmbedSigWord            = 0x0000;
            vol.mdb.drEmbedExtent.xdrStABN    = 0;
            vol.mdb.drEmbedExtent.xdrNumABlks = 0;

            /* vol.mdb.drXTFlSize */
            /* vol.mdb.drCTFlSize */

            /* vol.mdb.drXTExtRec[0..2] */
            /* vol.mdb.drCTExtRec[0..2] */

            vol.flags |= HFS_VOL_UPDATE_MDB | HFS_VOL_UPDATE_ALTMDB;

            /* initialize volume bitmap */

            vol.vbm = new byte[vol.vbmsz & 0xffff][];
            for (int i = 0; i < (vol.vbmsz & 0xffff); ++i)
                vol.vbm[i] = new byte[HFS_BLOCKSZ];

            vol.flags |= HFS_VOL_UPDATE_VBM;

            /* perform initial bad block sparing */

            if (nbadblocks > 0)
            {
                if (nbadblocks * 4 > (int) vol.vlen)
                {
                    fail(EINVAL, "volume contains too many bad blocks");
                    throw new HfsException(hfsErrno, hfsError);
                }

                badalloc = new int[nbadblocks];

                if ((vol.mdb.drNmAlBlks & 0xffff) == 1594)
                {
                    vol.mdb.drFreeBks =
                        (short) ((vol.mdb.drNmAlBlks & 0xffff) - 1);
                    vol.mdb.drNmAlBlks = vol.mdb.drFreeBks;
                }

                for (int i = 0; i < nbadblocks; ++i)
                {
                    long bnum = badblocks[i];
                    int anum;

                    if (bnum < (vol.mdb.drAlBlSt & 0xffff) || bnum == vol.vlen - 2)
                    {
                        fail(EINVAL, "can't spare critical bad block");
                        throw new HfsException(hfsErrno, hfsError);
                    }
                    else if (bnum >= vol.vlen)
                    {
                        fail(EINVAL, "bad block not in volume");
                        throw new HfsException(hfsErrno, hfsError);
                    }

                    anum = (int) ((bnum - (vol.mdb.drAlBlSt & 0xffff)) / vol.lpa);

                    if (anum < (vol.mdb.drNmAlBlks & 0xffff))
                        bmset(vol.vbm, anum);

                    badalloc[i] = anum;
                }

                vol.mdb.drAtrb |= HFS_ATRB_BBSPARED;
            }

            /* create extents overflow file */

            HfsNode.n_init(ext.hdrnd, ext, ndHdrNode, 0);

            ext.hdrnd.nnum       = 0;
            ext.hdrnd.nd.ndNRecs = 3;
            ext.hdrnd.roff[1]    = 0x078;
            ext.hdrnd.roff[2]    = 0x0f8;
            ext.hdrnd.roff[3]    = 0x1f8;

            Arrays.fill(ext.hdrnd.data, 14, 14 + 128, (byte) 0);

            ext.hdr.bthDepth    = 0;
            ext.hdr.bthRoot     = 0;
            ext.hdr.bthNRecs    = 0;
            ext.hdr.bthFNode    = 0;
            ext.hdr.bthLNode    = 0;
            ext.hdr.bthNodeSize = HFS_BLOCKSZ;
            ext.hdr.bthKeyLen   = 0x07;
            ext.hdr.bthNNodes   = 0;
            ext.hdr.bthFree     = 0;
            for (int i = 0; i < 76; ++i)
                ext.hdr.bthResv[i] = 0;

            ext.map = new byte[HFS_MAP1SZ];
            Arrays.fill(ext.map, (byte) 0);
            bmset1d(ext.map, 0);

            ext.mapsz = HFS_MAP1SZ;
            ext.flags = HFS_BT_UPDATE_HDR;

            /* create catalog file */

            HfsNode.n_init(cat.hdrnd, cat, ndHdrNode, 0);

            cat.hdrnd.nnum       = 0;
            cat.hdrnd.nd.ndNRecs = 3;
            cat.hdrnd.roff[1]    = 0x078;
            cat.hdrnd.roff[2]    = 0x0f8;
            cat.hdrnd.roff[3]    = 0x1f8;

            Arrays.fill(cat.hdrnd.data, 14, 14 + 128, (byte) 0);

            cat.hdr.bthDepth    = 0;
            cat.hdr.bthRoot     = 0;
            cat.hdr.bthNRecs    = 0;
            cat.hdr.bthFNode    = 0;
            cat.hdr.bthLNode    = 0;
            cat.hdr.bthNodeSize = HFS_BLOCKSZ;
            cat.hdr.bthKeyLen   = 0x25;
            cat.hdr.bthNNodes   = 0;
            cat.hdr.bthFree     = 0;
            for (int i = 0; i < 76; ++i)
                cat.hdr.bthResv[i] = 0;

            cat.map = new byte[HFS_MAP1SZ];
            Arrays.fill(cat.map, (byte) 0);
            bmset1d(cat.map, 0);

            cat.mapsz = HFS_MAP1SZ;
            cat.flags = HFS_BT_UPDATE_HDR;

            /* allocate space for header nodes (and initial extents) */

            if (HfsBTree.bt_space(ext, 1) == -1 ||
                HfsBTree.bt_space(cat, 1) == -1)
                throw new HfsException(hfsErrno, hfsError);

            --ext.hdr.bthFree;
            --cat.hdr.bthFree;

            /* create extent records for bad blocks */

            if (nbadblocks > 0)
            {
                HfsFileHandle bbfile = new HfsFileHandle();
                ExtDescriptor extent = new ExtDescriptor();
                ExtDataRec[] extrec = new ExtDataRec[1];
                ExtKeyRec key2 = new ExtKeyRec();
                byte[] record2 = new byte[HFS_MAX_EXTRECLEN];
                int[] reclen2 = new int[1];

                HfsFile.f_init(bbfile, vol, HFS_CNID_BADALLOC, "bad blocks");

                /* sort bad blocks */
                Integer[] boxed = new Integer[badalloc.length];
                for (int i = 0; i < badalloc.length; ++i)
                    boxed[i] = badalloc[i];
                Arrays.sort(boxed, Hfs::compare);
                for (int i = 0; i < badalloc.length; ++i)
                    badalloc[i] = boxed[i];

                for (int i = 0; i < nbadblocks; ++i)
                {
                    if (i == 0 || badalloc[i] != extent.xdrStABN)
                    {
                        extent.xdrStABN    = (short) badalloc[i];
                        extent.xdrNumABlks = 1;

                        if (extent.xdrStABN < (vol.mdb.drNmAlBlks & 0xffff) &&
                            HfsFile.f_addextent(bbfile, extent) == -1)
                            throw new HfsException(hfsErrno, hfsError);
                    }
                }

                /* flush local extents into extents overflow file */

                HfsFile.f_getptrs(bbfile, extrec, null, null);

                HfsRecord.r_makeextkey(key2, bbfile.fork,
                                       bbfile.cat.filFlNum, 0);
                HfsRecord.r_packextrec(key2, extrec[0], record2, reclen2);

                if (HfsBTree.bt_insert(vol.ext, record2, reclen2[0]) == -1)
                    throw new HfsException(hfsErrno, hfsError);
            }

            vol.flags |= HFS_VOL_MOUNTED;

            /* create root directory */

            if (HfsVolume.v_mkdir(vol, HFS_CNID_ROOTPAR, vname) == -1)
                throw new HfsException(hfsErrno, hfsError);

            vol.mdb.drNxtCNID = 16;  /* first CNID not reserved by Apple */

            /* write boot blocks */

            if (HfsMedium.m_zerobb(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);

            /* zero other unused space, if requested */

            if ((vol.flags & HFS_OPT_ZERO) != 0)
            {
                byte[] b = new byte[HFS_BLOCKSZ];
                long bnum;

                /* between MDB and VBM (never) */

                for (bnum = 3; bnum < (vol.mdb.drVBMSt & 0xffff); ++bnum)
                    HfsLow.b_writelb(vol, bnum, b);

                /* between VBM and first allocation block (sometimes if HFS_OPT_2048) */

                for (bnum = (vol.mdb.drVBMSt & 0xffff) + (vol.vbmsz & 0xffff);
                     bnum < (vol.mdb.drAlBlSt & 0xffff); ++bnum)
                    HfsLow.b_writelb(vol, bnum, b);

                /* between last allocation block and alternate MDB (sometimes) */

                for (bnum = (vol.mdb.drAlBlSt & 0xffff)
                            + (vol.mdb.drNmAlBlks & 0xffff) * vol.lpa;
                     bnum < vol.vlen - 2; ++bnum)
                    HfsLow.b_writelb(vol, bnum, b);

                /* final block (always) */

                HfsLow.b_writelb(vol, vol.vlen - 1, b);
            }

            /* flush remaining state and close volume */

            if (HfsVolume.v_close(vol) == -1)
                throw new HfsException(hfsErrno, hfsError);
        }
        catch (HfsException e)
        {
            HfsVolume.v_close(vol);
            throw e;
        }
    }

    /* -----------------------------------------------------------------------
     * Volume bitmap helpers (mirrors HfsVolume.bmtst/bmset/bmclr)
     * ----------------------------------------------------------------------- */

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

    private static void bmset1d(byte[] bm, int num)
    {
        bm[num >> 3] |= (byte) (0x80 >> (num & 0x07));
    }
}
