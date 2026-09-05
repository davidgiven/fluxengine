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
 * $Id: record.c,v 1.9 1998/11/02 22:09:07 rob Exp $
 */

package org.mars.hfsutils;

public final class HfsRecord
{
    private HfsRecord()
    {
    }

    /*
     * NAME:	record->packcatkey()
     * DESCRIPTION:	pack a catalog record key
     */
    public static void r_packcatkey(CatKeyRec key, byte[] pkey, int[] len)
    {
        int[] cursor = {0};

        HfsData.d_storesb(pkey, cursor, key.ckrKeyLen);
        HfsData.d_storesb(pkey, cursor, key.ckrResrv1);
        HfsData.d_storeul(pkey, cursor, key.ckrParID);

        HfsData.d_storestr(pkey, cursor, key.ckrCName, 32);

        if (len != null)
            len[0] = cursor[0];
    }

    /*
     * NAME:	record->unpackcatkey()
     * DESCRIPTION:	unpack a catalog record key
     */
    public static void r_unpackcatkey(byte[] pkey, int off, CatKeyRec key)
    {
        int[] cursor = {off};

        key.ckrKeyLen = HfsData.d_fetchsb(pkey, cursor);
        key.ckrResrv1 = HfsData.d_fetchsb(pkey, cursor);
        key.ckrParID = HfsData.d_fetchul(pkey, cursor);

        HfsData.d_fetchstr(pkey, cursor, key.ckrCName, 32);
    }

    /*
     * NAME:	record->packextkey()
     * DESCRIPTION:	pack an extents record key
     */
    public static void r_packextkey(ExtKeyRec key, byte[] pkey, int[] len)
    {
        int[] cursor = {0};

        HfsData.d_storesb(pkey, cursor, key.xkrKeyLen);
        HfsData.d_storesb(pkey, cursor, key.xkrFkType);
        HfsData.d_storeul(pkey, cursor, key.xkrFNum);
        HfsData.d_storeuw(pkey, cursor, key.xkrFABN);

        if (len != null)
            len[0] = ((1 + (pkey[0] & 0xff) + 1) & ~1);
    }

    /*
     * NAME:	record->unpackextkey()
     * DESCRIPTION:	unpack an extents record key
     */
    public static void r_unpackextkey(byte[] pkey, int off, ExtKeyRec key)
    {
        int[] cursor = {off};

        key.xkrKeyLen = HfsData.d_fetchsb(pkey, cursor);
        key.xkrFkType = HfsData.d_fetchsb(pkey, cursor);
        key.xkrFNum = HfsData.d_fetchul(pkey, cursor);
        key.xkrFABN =  (short) HfsData.d_fetchuw(pkey, cursor);
    }

    /*
     * NAME:	record->comparecatkeys()
     * DESCRIPTION:	compare two (packed) catalog record keys
     */
    public static int r_comparecatkeys(CatKeyRec key1, CatKeyRec key2)
    {
        int diff;

        diff = (int) (key1.ckrParID - key2.ckrParID);
        if (diff != 0)
            return diff;

        return HfsData.d_relstring(key1.ckrCName, key2.ckrCName);
    }

    /*
     * NAME:	record->compareextkeys()
     * DESCRIPTION:	compare two (packed) extents record keys
     */
    public static int r_compareextkeys(ExtKeyRec key1, ExtKeyRec key2)
    {
        int diff;

        diff = (int) (key1.xkrFNum - key2.xkrFNum);
        if (diff != 0)
            return diff;

        diff = (key1.xkrFkType & 0xff) - (key2.xkrFkType & 0xff);
        if (diff != 0)
            return diff;

        diff = key1.xkrFABN - key2.xkrFABN;
        return diff;
    }

    /*
     * NAME:	record->packcatdata()
     * DESCRIPTION:	pack catalog record data
     */
    public static void r_packcatdata(CatDataRec data, byte[] pdata, int[] len)
    {
        int[] cursor = {0};
        int startCursor;

        startCursor = cursor[0];

        HfsData.d_storesb(pdata, cursor, data.cdrType);
        HfsData.d_storesb(pdata, cursor, data.cdrResrv2);

        switch (data.cdrType)
        {
        case CatDataType.CDR_DIR_REC:
            HfsData.d_storesw(pdata, cursor, data.dirFlags);
            HfsData.d_storeuw(pdata, cursor, data.dirVal);
            HfsData.d_storeul(pdata, cursor, data.dirDirID);
            HfsData.d_storesl(pdata, cursor, data.dirCrDat);
            HfsData.d_storesl(pdata, cursor, data.dirMdDat);
            HfsData.d_storesl(pdata, cursor, data.dirBkDat);

            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frRect.top);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frRect.left);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frRect.bottom);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frRect.right);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frFlags);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frLocation.v);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frLocation.h);
            HfsData.d_storesw(pdata, cursor, data.dirUsrInfo.frView);

            HfsData.d_storesw(pdata, cursor, data.dirFndrInfo.frScroll.v);
            HfsData.d_storesw(pdata, cursor, data.dirFndrInfo.frScroll.h);
            HfsData.d_storesl(pdata, cursor, data.dirFndrInfo.frOpenChain);
            HfsData.d_storesw(pdata, cursor, data.dirFndrInfo.frUnused);
            HfsData.d_storesw(pdata, cursor, data.dirFndrInfo.frComment);
            HfsData.d_storesl(pdata, cursor, data.dirFndrInfo.frPutAway);

            for (int i = 0; i < 4; ++i)
                HfsData.d_storesl(pdata, cursor, data.dirResrv[i]);

            break;

        case CatDataType.CDR_FIL_REC:
            HfsData.d_storesb(pdata, cursor, data.filFlags);
            HfsData.d_storesb(pdata, cursor, data.filTyp);

            HfsData.d_storesl(pdata, cursor, data.filUsrWds.fdType);
            HfsData.d_storesl(pdata, cursor, data.filUsrWds.fdCreator);
            HfsData.d_storesw(pdata, cursor, data.filUsrWds.fdFlags);
            HfsData.d_storesw(pdata, cursor, data.filUsrWds.fdLocation.v);
            HfsData.d_storesw(pdata, cursor, data.filUsrWds.fdLocation.h);
            HfsData.d_storesw(pdata, cursor, data.filUsrWds.fdFldr);

            HfsData.d_storeul(pdata, cursor, data.filFlNum);

            HfsData.d_storeuw(pdata, cursor, data.filStBlk);
            HfsData.d_storeul(pdata, cursor, data.filLgLen);
            HfsData.d_storeul(pdata, cursor, data.filPyLen);

            HfsData.d_storeuw(pdata, cursor, data.filRStBlk);
            HfsData.d_storeul(pdata, cursor, data.filRLgLen);
            HfsData.d_storeul(pdata, cursor, data.filRPyLen);

            HfsData.d_storesl(pdata, cursor, data.filCrDat);
            HfsData.d_storesl(pdata, cursor, data.filMdDat);
            HfsData.d_storesl(pdata, cursor, data.filBkDat);

            HfsData.d_storesw(pdata, cursor, data.filFndrInfo.fdIconID);
            for (int i = 0; i < 4; ++i)
                HfsData.d_storesw(pdata, cursor, data.filFndrInfo.fdUnused[i]);
            HfsData.d_storesw(pdata, cursor, data.filFndrInfo.fdComment);
            HfsData.d_storesl(pdata, cursor, data.filFndrInfo.fdPutAway);

            HfsData.d_storeuw(pdata, cursor, data.filClpSize);

            for (int i = 0; i < 3; ++i)
            {
                HfsData.d_storeuw(pdata, cursor, data.filExtRec.data[i].xdrStABN);
                HfsData.d_storeuw(pdata, cursor, data.filExtRec.data[i].xdrNumABlks);
            }

            for (int i = 0; i < 3; ++i)
            {
                HfsData.d_storeuw(pdata, cursor, data.filRExtRec.data[i].xdrStABN);
                HfsData.d_storeuw(pdata, cursor, data.filRExtRec.data[i].xdrNumABlks);
            }

            HfsData.d_storesl(pdata, cursor, data.filResrv);

            break;

        case CatDataType.CDR_THD_REC:
            for (int i = 0; i < 2; ++i)
                HfsData.d_storesl(pdata, cursor, data.thdResrv[i]);

            HfsData.d_storeul(pdata, cursor, data.thdParID);

            HfsData.d_storestr(pdata, cursor, data.thdCName, 32);

            break;

        case CatDataType.CDR_FTHD_REC:
            for (int i = 0; i < 2; ++i)
                HfsData.d_storesl(pdata, cursor, data.fthdResrv[i]);

            HfsData.d_storeul(pdata, cursor, data.fthdParID);

            HfsData.d_storestr(pdata, cursor, data.fthdCName, 32);

            break;

        default:
            break;
        }

        if (len != null)
            len[0] += cursor[0] - startCursor;
    }

    /*
     * NAME:	record->unpackcatdata()
     * DESCRIPTION:	unpack catalog record data
     */
    public static void r_unpackcatdata(byte[] pdata, int off, CatDataRec data)
    {
        int[] cursor = {off};

        data.cdrType = HfsData.d_fetchsb(pdata, cursor);
        data.cdrResrv2 = HfsData.d_fetchsb(pdata, cursor);

        switch (data.cdrType)
        {
        case CatDataType.CDR_DIR_REC:
            data.dirFlags = HfsData.d_fetchsw(pdata, cursor);
            data.dirVal =  (short) HfsData.d_fetchuw(pdata, cursor);
            data.dirDirID = HfsData.d_fetchul(pdata, cursor);
            data.dirCrDat = HfsData.d_fetchsl(pdata, cursor);
            data.dirMdDat = HfsData.d_fetchsl(pdata, cursor);
            data.dirBkDat = HfsData.d_fetchsl(pdata, cursor);

            data.dirUsrInfo.frRect.top = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frRect.left = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frRect.bottom = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frRect.right = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frFlags = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frLocation.v = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frLocation.h = HfsData.d_fetchsw(pdata, cursor);
            data.dirUsrInfo.frView = HfsData.d_fetchsw(pdata, cursor);

            data.dirFndrInfo.frScroll.v = HfsData.d_fetchsw(pdata, cursor);
            data.dirFndrInfo.frScroll.h = HfsData.d_fetchsw(pdata, cursor);
            data.dirFndrInfo.frOpenChain = HfsData.d_fetchsl(pdata, cursor);
            data.dirFndrInfo.frUnused = HfsData.d_fetchsw(pdata, cursor);
            data.dirFndrInfo.frComment = HfsData.d_fetchsw(pdata, cursor);
            data.dirFndrInfo.frPutAway = HfsData.d_fetchsl(pdata, cursor);

            for (int i = 0; i < 4; ++i)
                data.dirResrv[i] = HfsData.d_fetchsl(pdata, cursor);

            break;

        case CatDataType.CDR_FIL_REC:
            data.filFlags = HfsData.d_fetchsb(pdata, cursor);
            data.filTyp = HfsData.d_fetchsb(pdata, cursor);

            data.filUsrWds.fdType = HfsData.d_fetchsl(pdata, cursor);
            data.filUsrWds.fdCreator = HfsData.d_fetchsl(pdata, cursor);
            data.filUsrWds.fdFlags = HfsData.d_fetchsw(pdata, cursor);
            data.filUsrWds.fdLocation.v = HfsData.d_fetchsw(pdata, cursor);
            data.filUsrWds.fdLocation.h = HfsData.d_fetchsw(pdata, cursor);
            data.filUsrWds.fdFldr = HfsData.d_fetchsw(pdata, cursor);

            data.filFlNum = HfsData.d_fetchul(pdata, cursor);

            data.filStBlk =  (short) HfsData.d_fetchuw(pdata, cursor);
            data.filLgLen = HfsData.d_fetchul(pdata, cursor);
            data.filPyLen = HfsData.d_fetchul(pdata, cursor);

            data.filRStBlk =  (short) HfsData.d_fetchuw(pdata, cursor);
            data.filRLgLen = HfsData.d_fetchul(pdata, cursor);
            data.filRPyLen = HfsData.d_fetchul(pdata, cursor);

            data.filCrDat = HfsData.d_fetchsl(pdata, cursor);
            data.filMdDat = HfsData.d_fetchsl(pdata, cursor);
            data.filBkDat = HfsData.d_fetchsl(pdata, cursor);

            data.filFndrInfo.fdIconID = HfsData.d_fetchsw(pdata, cursor);
            for (int i = 0; i < 4; ++i)
                data.filFndrInfo.fdUnused[i] = HfsData.d_fetchsw(pdata, cursor);
            data.filFndrInfo.fdComment = HfsData.d_fetchsw(pdata, cursor);
            data.filFndrInfo.fdPutAway = HfsData.d_fetchsl(pdata, cursor);

            data.filClpSize =  (short) HfsData.d_fetchuw(pdata, cursor);

            for (int i = 0; i < 3; ++i)
            {
                data.filExtRec.data[i].xdrStABN =  (short) HfsData.d_fetchuw(pdata, cursor);
                data.filExtRec.data[i].xdrNumABlks =  (short) HfsData.d_fetchuw(pdata, cursor);
            }

            for (int i = 0; i < 3; ++i)
            {
                data.filRExtRec.data[i].xdrStABN =  (short) HfsData.d_fetchuw(pdata, cursor);
                data.filRExtRec.data[i].xdrNumABlks =  (short) HfsData.d_fetchuw(pdata, cursor);
            }

            data.filResrv = HfsData.d_fetchsl(pdata, cursor);

            break;

        case CatDataType.CDR_THD_REC:
            for (int i = 0; i < 2; ++i)
                data.thdResrv[i] = HfsData.d_fetchsl(pdata, cursor);

            data.thdParID = HfsData.d_fetchul(pdata, cursor);

            HfsData.d_fetchstr(pdata, cursor, data.thdCName, 32);

            break;

        case CatDataType.CDR_FTHD_REC:
            for (int i = 0; i < 2; ++i)
                data.fthdResrv[i] = HfsData.d_fetchsl(pdata, cursor);

            data.fthdParID = HfsData.d_fetchul(pdata, cursor);

            HfsData.d_fetchstr(pdata, cursor, data.fthdCName, 32);

            break;

        default:
            break;
        }
    }

    /*
     * NAME:	record->packextdata()
     * DESCRIPTION:	pack extent record data
     */
    public static void r_packextdata(ExtDataRec data, byte[] pdata, int[] len)
    {
        int[] cursor = {0};
        int startCursor;

        startCursor = cursor[0];

        for (int i = 0; i < 3; ++i)
        {
            HfsData.d_storeuw(pdata, cursor, data.data[i].xdrStABN);
            HfsData.d_storeuw(pdata, cursor, data.data[i].xdrNumABlks);
        }

        if (len != null)
            len[0] += cursor[0] - startCursor;
    }

    /*
     * NAME:	record->unpackextdata()
     * DESCRIPTION:	unpack extent record data
     */
    public static void r_unpackextdata(byte[] pdata, int off, ExtDataRec data)
    {
        int[] cursor = {off};

        for (int i = 0; i < 3; ++i)
        {
            data.data[i].xdrStABN =  (short) HfsData.d_fetchuw(pdata, cursor);
            data.data[i].xdrNumABlks =  (short) HfsData.d_fetchuw(pdata, cursor);
        }
    }

    /*
     * NAME:	record->makecatkey()
     * DESCRIPTION:	construct a catalog record key
     */
    public static void r_makecatkey(CatKeyRec key, long parid, String name)
    {
        int realLen = name.indexOf(0);
        if (realLen == -1)
            realLen = name.length();

        /* Catalog key length is fixed: ckrCName is always a full Str31 (31
         * characters), so the on-disk key record is 38 bytes and ckrKeyLen
         * is the constant 0x05 + 31 + 1 = 37 (giving recKeySkip = 38). */
        int len = 31;  /* HFS_MAX_CATNAMELEN */

        key.ckrKeyLen = (byte) (0x05 + len + (len & 1));
        key.ckrResrv1 = 0;
        key.ckrParID  = parid;

        for (int i = 0; i < realLen; i++)
            key.ckrCName[i] = name.charAt(i);
        key.ckrCName[realLen] = 0;
    }

    /*
     * NAME:	record->makeextkey()
     * DESCRIPTION:	construct an extents record key
     */
    public static void r_makeextkey(ExtKeyRec key,
                                    int fork, long fnum, int fabn)
    {
        key.xkrKeyLen = 0x07;
        key.xkrFkType = (byte) fork;
        key.xkrFNum   = fnum;
        key.xkrFABN   = (short) fabn;
    }

    /*
     * NAME:	record->packcatrec()
     * DESCRIPTION:	create a packed catalog record
     *
     * In C, r_packcatdata is called with HFS_RECDATA(precord) which is
     * precord + HFS_RECKEYSKIP(precord) (pointer arithmetic).  In Java we
     * use a single cursor that advances through the key then the data.
     */
    public static void r_packcatrec(CatKeyRec key, CatDataRec data,
                                    byte[] precord, int[] len)
    {
        int[] cursor = {0};

        /* pack key */
        HfsData.d_storesb(precord, cursor, key.ckrKeyLen);
        HfsData.d_storesb(precord, cursor, key.ckrResrv1);
        HfsData.d_storeul(precord, cursor, key.ckrParID);
        HfsData.d_storestr(precord, cursor, key.ckrCName, 32);

        /* pack data at HFS_RECDATA offset */
        int startCursor = cursor[0];

        HfsData.d_storesb(precord, cursor, data.cdrType);
        HfsData.d_storesb(precord, cursor, data.cdrResrv2);

        switch (data.cdrType)
        {
        case CatDataType.CDR_DIR_REC:
            HfsData.d_storesw(precord, cursor, data.dirFlags);
            HfsData.d_storeuw(precord, cursor, data.dirVal);
            HfsData.d_storeul(precord, cursor, data.dirDirID);
            HfsData.d_storesl(precord, cursor, data.dirCrDat);
            HfsData.d_storesl(precord, cursor, data.dirMdDat);
            HfsData.d_storesl(precord, cursor, data.dirBkDat);

            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frRect.top);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frRect.left);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frRect.bottom);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frRect.right);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frFlags);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frLocation.v);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frLocation.h);
            HfsData.d_storesw(precord, cursor, data.dirUsrInfo.frView);

            HfsData.d_storesw(precord, cursor, data.dirFndrInfo.frScroll.v);
            HfsData.d_storesw(precord, cursor, data.dirFndrInfo.frScroll.h);
            HfsData.d_storesl(precord, cursor, data.dirFndrInfo.frOpenChain);
            HfsData.d_storesw(precord, cursor, data.dirFndrInfo.frUnused);
            HfsData.d_storesw(precord, cursor, data.dirFndrInfo.frComment);
            HfsData.d_storesl(precord, cursor, data.dirFndrInfo.frPutAway);

            for (int i = 0; i < 4; ++i)
                HfsData.d_storesl(precord, cursor, data.dirResrv[i]);

            break;

        case CatDataType.CDR_FIL_REC:
            HfsData.d_storesb(precord, cursor, data.filFlags);
            HfsData.d_storesb(precord, cursor, data.filTyp);

            HfsData.d_storesl(precord, cursor, data.filUsrWds.fdType);
            HfsData.d_storesl(precord, cursor, data.filUsrWds.fdCreator);
            HfsData.d_storesw(precord, cursor, data.filUsrWds.fdFlags);
            HfsData.d_storesw(precord, cursor, data.filUsrWds.fdLocation.v);
            HfsData.d_storesw(precord, cursor, data.filUsrWds.fdLocation.h);
            HfsData.d_storesw(precord, cursor, data.filUsrWds.fdFldr);

            HfsData.d_storeul(precord, cursor, data.filFlNum);

            HfsData.d_storeuw(precord, cursor, data.filStBlk);
            HfsData.d_storeul(precord, cursor, data.filLgLen);
            HfsData.d_storeul(precord, cursor, data.filPyLen);

            HfsData.d_storeuw(precord, cursor, data.filRStBlk);
            HfsData.d_storeul(precord, cursor, data.filRLgLen);
            HfsData.d_storeul(precord, cursor, data.filRPyLen);

            HfsData.d_storesl(precord, cursor, data.filCrDat);
            HfsData.d_storesl(precord, cursor, data.filMdDat);
            HfsData.d_storesl(precord, cursor, data.filBkDat);

            HfsData.d_storesw(precord, cursor, data.filFndrInfo.fdIconID);
            for (int i = 0; i < 4; ++i)
                HfsData.d_storesw(precord, cursor, data.filFndrInfo.fdUnused[i]);
            HfsData.d_storesw(precord, cursor, data.filFndrInfo.fdComment);
            HfsData.d_storesl(precord, cursor, data.filFndrInfo.fdPutAway);

            HfsData.d_storeuw(precord, cursor, data.filClpSize);

            for (int i = 0; i < 3; ++i)
            {
                HfsData.d_storeuw(precord, cursor, data.filExtRec.data[i].xdrStABN);
                HfsData.d_storeuw(precord, cursor, data.filExtRec.data[i].xdrNumABlks);
            }

            for (int i = 0; i < 3; ++i)
            {
                HfsData.d_storeuw(precord, cursor, data.filRExtRec.data[i].xdrStABN);
                HfsData.d_storeuw(precord, cursor, data.filRExtRec.data[i].xdrNumABlks);
            }

            HfsData.d_storesl(precord, cursor, data.filResrv);

            break;

        case CatDataType.CDR_THD_REC:
            for (int i = 0; i < 2; ++i)
                HfsData.d_storesl(precord, cursor, data.thdResrv[i]);

            HfsData.d_storeul(precord, cursor, data.thdParID);

            HfsData.d_storestr(precord, cursor, data.thdCName, 32);

            break;

        case CatDataType.CDR_FTHD_REC:
            for (int i = 0; i < 2; ++i)
                HfsData.d_storesl(precord, cursor, data.fthdResrv[i]);

            HfsData.d_storeul(precord, cursor, data.fthdParID);

            HfsData.d_storestr(precord, cursor, data.fthdCName, 32);

            break;

        default:
            break;
        }

        if (len != null)
            len[0] = cursor[0];
    }

    /*
     * NAME:	record->packextrec()
     * DESCRIPTION:	create a packed extents record
     */
    public static void r_packextrec(ExtKeyRec key, ExtDataRec data,
                                    byte[] precord, int[] len)
    {
        int[] cursor = {0};

        /* pack key */
        HfsData.d_storesb(precord, cursor, key.xkrKeyLen);
        HfsData.d_storesb(precord, cursor, key.xkrFkType);
        HfsData.d_storeul(precord, cursor, key.xkrFNum);
        HfsData.d_storeuw(precord, cursor, key.xkrFABN);

        /* pack data at HFS_RECDATA offset */
        int startCursor = cursor[0];

        for (int i = 0; i < 3; ++i)
        {
            HfsData.d_storeuw(precord, cursor, data.data[i].xdrStABN);
            HfsData.d_storeuw(precord, cursor, data.data[i].xdrNumABlks);
        }

        if (len != null)
            len[0] = cursor[0];
    }

    /*
     * NAME:	record->packdirent()
     * DESCRIPTION:	make changes to a catalog record
     */
    public static void r_packdirent(CatDataRec data, HfsDirEnt ent)
    {
        switch (data.cdrType)
        {
        case CatDataType.CDR_DIR_REC:
            data.dirCrDat = HfsData.d_mtime(ent.crdate);
            data.dirMdDat = HfsData.d_mtime(ent.mddate);
            data.dirBkDat = HfsData.d_mtime(ent.bkdate);

            data.dirUsrInfo.frFlags      = ent.fdflags;
            data.dirUsrInfo.frLocation.v = ent.fdlocation.v;
            data.dirUsrInfo.frLocation.h = ent.fdlocation.h;

            data.dirUsrInfo.frRect.top    = ent.uDir.rect.top;
            data.dirUsrInfo.frRect.left   = ent.uDir.rect.left;
            data.dirUsrInfo.frRect.bottom = ent.uDir.rect.bottom;
            data.dirUsrInfo.frRect.right  = ent.uDir.rect.right;

            break;

        case CatDataType.CDR_FIL_REC:
            if ((ent.flags & HfsConstants.HFS_ISLOCKED) != 0)
                data.filFlags |=  (1 << 0);
            else
                data.filFlags &= ~(1 << 0);

            data.filCrDat = HfsData.d_mtime(ent.crdate);
            data.filMdDat = HfsData.d_mtime(ent.mddate);
            data.filBkDat = HfsData.d_mtime(ent.bkdate);

            data.filUsrWds.fdFlags      = ent.fdflags;
            data.filUsrWds.fdLocation.v = ent.fdlocation.v;
            data.filUsrWds.fdLocation.h = ent.fdlocation.h;

            data.filUsrWds.fdType =
                HfsData.d_getsl(new byte[] {
                    (byte) ent.uFile.type[0], (byte) ent.uFile.type[1],
                    (byte) ent.uFile.type[2], (byte) ent.uFile.type[3]
                }, 0);
            data.filUsrWds.fdCreator =
                HfsData.d_getsl(new byte[] {
                    (byte) ent.uFile.creator[0], (byte) ent.uFile.creator[1],
                    (byte) ent.uFile.creator[2], (byte) ent.uFile.creator[3]
                }, 0);

            break;
        }
    }

    /*
     * NAME:	record->unpackdirent()
     * DESCRIPTION:	unpack catalog information into hfsdirent structure
     */
    public static void r_unpackdirent(long parid, String name,
                                      CatDataRec data, HfsDirEnt ent)
    {
        int nameLen = name.indexOf(0);
        if (nameLen == -1)
            nameLen = name.length();

        for (int i = 0; i < nameLen; i++)
            ent.name[i] = name.charAt(i);
        ent.name[nameLen] = 0;
        ent.parid = parid;

        switch (data.cdrType)
        {
        case CatDataType.CDR_DIR_REC:
            ent.flags = HfsConstants.HFS_ISDIR;
            ent.cnid  = data.dirDirID;

            ent.crdate = HfsData.d_ltime(data.dirCrDat);
            ent.mddate = HfsData.d_ltime(data.dirMdDat);
            ent.bkdate = HfsData.d_ltime(data.dirBkDat);

            ent.fdflags      = data.dirUsrInfo.frFlags;
            ent.fdlocation.v = data.dirUsrInfo.frLocation.v;
            ent.fdlocation.h = data.dirUsrInfo.frLocation.h;

            ent.uDir.valence = data.dirVal;

            ent.uDir.rect.top    = data.dirUsrInfo.frRect.top;
            ent.uDir.rect.left   = data.dirUsrInfo.frRect.left;
            ent.uDir.rect.bottom = data.dirUsrInfo.frRect.bottom;
            ent.uDir.rect.right  = data.dirUsrInfo.frRect.right;

            break;

        case CatDataType.CDR_FIL_REC:
            ent.flags = (data.filFlags & (1 << 0)) != 0
                ? HfsConstants.HFS_ISLOCKED : 0;
            ent.cnid  = data.filFlNum;

            ent.crdate = HfsData.d_ltime(data.filCrDat);
            ent.mddate = HfsData.d_ltime(data.filMdDat);
            ent.bkdate = HfsData.d_ltime(data.filBkDat);

            ent.fdflags      = data.filUsrWds.fdFlags;
            ent.fdlocation.v = data.filUsrWds.fdLocation.v;
            ent.fdlocation.h = data.filUsrWds.fdLocation.h;

            ent.uFile.dsize = data.filLgLen;
            ent.uFile.rsize = data.filRLgLen;

            byte[] typeBytes = new byte[4];
            HfsData.d_putsl(typeBytes, 0, data.filUsrWds.fdType);
            ent.uFile.type[0] = (char) (typeBytes[0] & 0xff);
            ent.uFile.type[1] = (char) (typeBytes[1] & 0xff);
            ent.uFile.type[2] = (char) (typeBytes[2] & 0xff);
            ent.uFile.type[3] = (char) (typeBytes[3] & 0xff);

            byte[] creatorBytes = new byte[4];
            HfsData.d_putsl(creatorBytes, 0, data.filUsrWds.fdCreator);
            ent.uFile.creator[0] = (char) (creatorBytes[0] & 0xff);
            ent.uFile.creator[1] = (char) (creatorBytes[1] & 0xff);
            ent.uFile.creator[2] = (char) (creatorBytes[2] & 0xff);
            ent.uFile.creator[3] = (char) (creatorBytes[3] & 0xff);

            ent.uFile.type[4] = ent.uFile.creator[4] = 0;

            break;
        }
    }
}
