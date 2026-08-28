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
 * $Id: apple.h,v 1.1 1998/04/11 08:27:11 rob Exp $
 */

package org.mars.hfsutils;

public class CatDataRec {

    /* record type */
    public byte cdrType;

    /* reserved */
    public byte cdrResrv2;

    /* dir variant */

    /* directory flags */
    public short dirFlags;

    /* directory valence */
    public short dirVal;

    /* directory ID */
    public long dirDirID;

    /* date and time of creation */
    public long dirCrDat;

    /* date and time of last modification */
    public long dirMdDat;

    /* date and time of last backup */
    public long dirBkDat;

    /* Finder information */
    public DInfo dirUsrInfo = new DInfo();

    /* additional Finder information */
    public DXInfo dirFndrInfo = new DXInfo();

    /* reserved */
    public long[] dirResrv = new long[4];

    /* fil variant */

    /* file flags */
    public byte filFlags;

    /* file type */
    public byte filTyp;

    /* Finder information */
    public FInfo filUsrWds = new FInfo();

    /* file ID */
    public long filFlNum;

    /* first alloc block of data fork */
    public short filStBlk;

    /* logical EOF of data fork */
    public long filLgLen;

    /* physical EOF of data fork */
    public long filPyLen;

    /* first alloc block of resource fork */
    public short filRStBlk;

    /* logical EOF of resource fork */
    public long filRLgLen;

    /* physical EOF of resource fork */
    public long filRPyLen;

    /* date and time of creation */
    public long filCrDat;

    /* date and time of last modification */
    public long filMdDat;

    /* date and time of last backup */
    public long filBkDat;

    /* additional Finder information */
    public FXInfo filFndrInfo = new FXInfo();

    /* file clump size */
    public short filClpSize;

    /* first data fork extent record */
    public ExtDataRec filExtRec = new ExtDataRec();

    /* first resource fork extent record */
    public ExtDataRec filRExtRec = new ExtDataRec();

    /* reserved */
    public long filResrv;

    /* dthd variant */

    /* reserved */
    public long[] thdResrv = new long[2];

    /* parent ID for this directory */
    public long thdParID;

    /* name of this directory */
    public char[] thdCName = new char[32];

    /* fthd variant */

    /* reserved */
    public long[] fthdResrv = new long[2];

    /* parent ID for this file */
    public long fthdParID;

    /* name of this file */
    public char[] fthdCName = new char[32];
}
