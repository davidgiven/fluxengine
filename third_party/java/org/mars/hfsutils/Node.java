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

public class Node {

    /* btree to which this node belongs */
    public BTree bt;

    /* node index */
    public long nnum;

    /* node descriptor */
    public NodeDescriptor nd = new NodeDescriptor();

    /* current record index */
    public int rnum;

    /* record offsets */
    public int[] roff = new int[36];

    /* raw contents of node */
    public byte[] data = new byte[512];
}
