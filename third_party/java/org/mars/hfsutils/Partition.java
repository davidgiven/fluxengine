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

public class Partition {

    /* partition signature (0x504d or 0x5453) */
    public short pmSig;

    /* reserved */
    public short pmSigPad;

    /* number of blocks in partition map */
    public int pmMapBlkCnt;

    /* first physical block of partition */
    public int pmPyPartStart;

    /* number of blocks in partition */
    public int pmPartBlkCnt;

    /* partition name */
    public char[] pmPartName = new char[33];

    /* partition type */
    public char[] pmParType = new char[33];

    /* first logical block of data area */
    public int pmLgDataStart;

    /* number of blocks in data area */
    public int pmDataCnt;

    /* partition status information */
    public int pmPartStatus;

    /* first logical block of boot code */
    public int pmLgBootStart;

    /* size of boot code, in bytes */
    public int pmBootSize;

    /* boot code load address */
    public int pmBootAddr;

    /* reserved */
    public int pmBootAddr2;

    /* boot code entry point */
    public int pmBootEntry;

    /* reserved */
    public int pmBootEntry2;

    /* boot code checksum */
    public int pmBootCksum;

    /* processor type */
    public char[] pmProcessor = new char[17];

    /* reserved */
    public short[] pmPad = new short[188];
}
