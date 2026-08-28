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

public class Mdb {

    /* volume signature (0x4244 for HFS) */
    public short drSigWord;

    /* date and time of volume creation */
    public long drCrDate;

    /* date and time of last modification */
    public long drLsMod;

    /* volume attributes */
    public short drAtrb;

    /* number of files in root directory */
    public short drNmFls;

    /* first block of volume bit map (always 3) */
    public short drVBMSt;

    /* start of next allocation search */
    public short drAllocPtr;

    /* number of allocation blocks in volume */
    public short drNmAlBlks;

    /* size (in bytes) of allocation blocks */
    public long drAlBlkSiz;

    /* default clump size */
    public long drClpSiz;

    /* first allocation block in volume */
    public short drAlBlSt;

    /* next unused catalog node ID (dir/file ID) */
    public long drNxtCNID;

    /* number of unused allocation blocks */
    public short drFreeBks;

    /* volume name (1-27 chars) */
    public char[] drVN = new char[28];

    /* date and time of last backup */
    public long drVolBkUp;

    /* volume backup sequence number */
    public short drVSeqNum;

    /* volume write count */
    public long drWrCnt;

    /* clump size for extents overflow file */
    public long drXTClpSiz;

    /* clump size for catalog file */
    public long drCTClpSiz;

    /* number of directories in root directory */
    public short drNmRtDirs;

    /* number of files in volume */
    public long drFilCnt;

    /* number of directories in volume */
    public long drDirCnt;

    /* information used by the Finder */
    public long[] drFndrInfo = new long[8];

    /* type of embedded volume */
    public short drEmbedSigWord;

    /* location of embedded volume */
    public ExtDescriptor drEmbedExtent = new ExtDescriptor();

    /* size (in bytes) of extents overflow file */
    public long drXTFlSize;

    /* first extent record for extents file */
    public ExtDataRec drXTExtRec = new ExtDataRec();

    /* size (in bytes) of catalog file */
    public long drCTFlSize;

    /* first extent record for catalog file */
    public ExtDataRec drCTExtRec = new ExtDataRec();
}
