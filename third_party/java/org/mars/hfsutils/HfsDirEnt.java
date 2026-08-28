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
 * $Id: hfs.h,v 1.11 1998/11/02 22:09:01 rob Exp $
 */

package org.mars.hfsutils;

public class HfsDirEnt {

    public static class Point {

        /* vertical coordinate */
        public short v;

        /* horizontal coordinate */
        public short h;
    }

    public static class Rect {

        /* top edge of rectangle */
        public short top;

        /* left edge */
        public short left;

        /* bottom edge */
        public short bottom;

        /* right edge */
        public short right;
    }

    public static class File {

        /* size of data fork */
        public long dsize;

        /* size of resource fork */
        public long rsize;

        /* file type code (plus null) */
        public char[] type = new char[5];

        /* file creator code (plus null) */
        public char[] creator = new char[5];
    }

    public static class Dir {

        /* number of items in directory */
        public int valence;

        /* directory's rectangle */
        public Rect rect = new Rect();
    }

    /* catalog name (MacOS Standard Roman) */
    public char[] name = new char[32];

    /* bit flags */
    public int flags;

    /* catalog node id (CNID) */
    public long cnid;

    /* CNID of parent directory */
    public long parid;

    /* date of creation */
    public long crdate;

    /* date of last modification */
    public long mddate;

    /* date of last backup */
    public long bkdate;

    /* Macintosh Finder flags */
    public short fdflags;

    /* Finder icon location */
    public Point fdlocation = new Point();

    /* file union member */
    public File uFile = new File();

    /* directory union member */
    public Dir uDir = new Dir();
}
