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
 */

package org.mars.hfsutils;

import java.io.IOException;

/**
 * Exception carrying the C {@code errno} and {@code hfs_error} string.
 */
public final class HfsException extends IOException
{
    /** POSIX errno value (EINVAL, ENOENT, EIO, etc.). */
    public final int errno;

    public HfsException(int errno, String message)
    {
        super(message);
        this.errno = errno;
    }

    public HfsException(String message)
    {
        this(0, message);
    }

    /* POSIX errno constants used by libhfs. */
    public static final int EINVAL  = 22;
    public static final int ENOENT  = 2;
    public static final int EIO     = 5;
    public static final int ENOMEM  = 12;
    public static final int EEXIST  = 17;
    public static final int ENOTDIR = 20;
    public static final int EISDIR  = 21;
    public static final int EROFS   = 30;
    public static final int ENOSPC  = 28;
    public static final int ENAMETOOLONG = 91;
    public static final int ENOTEMPTY = 39;
    public static final int EINTR  = 4;
}
