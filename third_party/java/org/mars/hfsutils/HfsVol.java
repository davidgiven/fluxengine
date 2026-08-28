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

import org.mars.hfsutils.os.HfsOs;

public class HfsVol {

    /* OS-dependent private descriptor data */
    public HfsOs priv;

    /* bit flags */
    public int flags;

    /* ordinal HFS partition number */
    public int pnum;

    /* logical block offset to start of volume */
    public long vstart;

    /* number of logical blocks in volume */
    public long vlen;

    /* number of logical blocks per allocation block */
    public int lpa;

    /* cache of recently used blocks */
    public Bcache cache;

    /* master directory block */
    public Mdb mdb = new Mdb();

    /* volume bitmap */
    public byte[][] vbm;

    /* number of blocks in bitmap */
    public short vbmsz;

    /* B*-tree control block for extents overflow file */
    public BTree ext = new BTree();

    /* B*-tree control block for catalog file */
    public BTree cat = new BTree();

    /* directory id of current working directory */
    public long cwd;

    /* number of external references to this volume */
    public int refs;

    /* list of open files */
    public HfsFileHandle files;

    /* list of open directories */
    public HfsDir dirs;

    public HfsVol prev;

    public HfsVol next;
}
