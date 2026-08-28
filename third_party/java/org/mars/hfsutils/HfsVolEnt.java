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
 * $Id: libhfs.h,v 1.7 1998/11/02 22:09:02 rob Exp $
 */

package org.mars.hfsutils;

public class HfsVolEnt {

    /* name of volume (MacOS Standard Roman) */
    public char[] name = new char[28];

    /* volume flags */
    public int flags;

    /* total bytes on volume */
    public long totbytes;

    /* free bytes on volume */
    public long freebytes;

    /* volume allocation block size */
    public long alblocksz;

    /* default file clump size */
    public long clumpsz;

    /* number of files in volume */
    public long numfiles;

    /* number of directories in volume */
    public long numdirs;

    /* volume creation date */
    public long crdate;

    /* last volume modification date */
    public long mddate;

    /* last volume backup date */
    public long bkdate;

    /* CNID of MacOS System Folder */
    public long blessed;
}
