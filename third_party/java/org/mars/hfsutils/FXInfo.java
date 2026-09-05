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

public class FXInfo {

    /* icon ID */
    public short fdIconID;

    /* reserved */
    public short[] fdUnused = new short[4];

    /* comment ID */
    public short fdComment;

    /* home directory ID */
    public long fdPutAway;
}
