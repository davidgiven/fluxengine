/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * Java port of os.h
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

package org.mars.hfsutils.os;

/**
 * Abstract OS-dependent I/O layer, translated from {@code os.h}.
 *
 * <p>In the C library, {@code os_open/close/seek/read/write} are
 * implemented by the consumer. Here the consumer provides an
 * {@code HfsOs} instance to {@link org.mars.hfsutils.Hfs#hfsMount}.
 *
 * <p>All block I/O is in 512-byte logical blocks.
 */
public abstract class HfsOs
{
    /**
     * Open the volume source (corresponds to {@code os_open}).
     *
     * @param path opaque identifier (may be unused in practice)
     * @param mode {@link org.mars.hfsutils.Hfs#HFS_MODE_RDONLY} etc.
     * @return 0 on success, -1 on error
     */
    public abstract int open(String path, int mode);

    /** Close the volume source ({@code os_close}). */
    public abstract int close();

    /**
     * Return non-zero if {@code path} refers to the same volume
     * as the one this device is already open on ({@code os_same}).
     */
    public abstract int same(String path);

    /**
     * Seek to a logical block number ({@code os_seek}).
     * If {@code offset} == -1, return the total number of blocks.
     *
     * @return the actual block position, or -1 on error
     */
    public abstract long seek(long offset);

    /**
     * Read {@code len} logical blocks ({@code os_read}).
     * {@code buf} has at least {@code len*512} bytes.
     *
     * @return number of blocks read, or -1 on error
     */
    public abstract long read(byte[] buf, long len);

    /**
     * Write {@code len} logical blocks ({@code os_write}).
     * {@code buf} has at least {@code len*512} bytes.
     *
     * @return number of blocks written, or -1 on error
     */
    public abstract long write(byte[] buf, long len);
}
