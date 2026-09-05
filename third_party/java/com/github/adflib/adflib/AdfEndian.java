/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  defendian.h + adf_defs.h (Short/Long/swapShort/swapLong)
 *
 *  $Id$
 *
 *  This file is part of ADFLib.
 *
 *  ADFLib is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  ADFLib is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Foobar; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

package com.github.adflib.adflib;

import java.nio.ByteBuffer;
import java.nio.ByteOrder;

/**
 * Endian helpers translated from {@code defendian.h} and {@code adf_defs.h}.
 *
 * <p>Amiga ADF structures are big-endian (Motorola 68k). {@code adf_defs.h}
 * defines:
 * <pre>
 *   #define Short(p) ((p)[0] << 8 | (p)[1])
 *   #define Long(p) (Short(p) << 16 | Short(p + 2))
 *   #define swapShort(p) ((p)[0] << 8 | (p)[1])
 *   #define swapLong(p) (swapShort(p) << 16 | swapShort(p + 2))
 * </pre>
 * All are big-endian loads. {@code defendian.h} defines {@code LITT_ENDIAN}
 * on little-endian hosts, where the C code would byte-swap; in Java we always
 * expose explicit big- and little-endian helpers via {@link ByteBuffer}.
 *
 * <p>ByteBuffer helpers use absolute {@code get(int)}/{@code put(int,byte)}
 * without touching {@code position}/{@code limit}, and honour
 * {@link ByteOrder#BIG_ENDIAN} for ADF data. Use {@code hasArray()} fast-path
 * where callers prefer {@code arraycopy}.
 */
public final class AdfEndian
{

    private AdfEndian()
    {
    }

    /* adf_defs.h — macro Short(p) : big-endian 16-bit from byte pointer */

    public static int Short(byte[] p, int off)
    {
        return ((p[off] & 0xFF) << 8) | (p[off + 1] & 0xFF);
    }

    /* adf_defs.h — macro Long(p) : big-endian 32-bit from byte pointer */

    public static int Long(byte[] p, int off)
    {
        return (Short(p, off) << 16) | (Short(p, off + 2) & 0xFFFF);
    }

    /* swap short and swap long macros for little endian machines — same big-endian load */

    public static int swapShort(byte[] p, int off)
    {
        return ((p[off] & 0xFF) << 8) | (p[off + 1] & 0xFF);
    }

    public static int swapLong(byte[] p, int off)
    {
        return (swapShort(p, off) << 16) | (swapShort(p, off + 2) & 0xFFFF);
    }

    /* unsigned variants */

    public static int ShortU(byte[] p, int off)
    {
        return Short(p, off) & 0xFFFF;
    }

    public static long LongU(byte[] p, int off)
    {
        return Long(p, off) & 0xFFFFFFFFL;
    }

    /* ByteBuffer — big-endian (ADF native) — absolute, no position/limit side-effects */

    public static int ld_16be(ByteBuffer buf, int off)
    {
        /* big-endian 16-bit */
        return buf.getShort(off) & 0xFFFF;
    }

    public static long ld_32be(ByteBuffer buf, int off)
    {
        /* big-endian 32-bit */
        return buf.getInt(off) & 0xFFFFFFFFL;
    }

    public static int ld_32beSigned(ByteBuffer buf, int off)
    {
        /* big-endian 32-bit signed */
        return buf.getInt(off);
    }

    public static void st_16be(ByteBuffer buf, int off, int val)
    {
        /* Store a 2-byte word in big-endian */
        buf.putShort(off, (short) (val & 0xFFFF));
    }

    public static void st_32be(ByteBuffer buf, int off, int val)
    {
        /* Store a 4-byte word in big-endian */
        buf.putInt(off, val);
    }

    public static void st_32be(ByteBuffer buf, int off, long val)
    {
        /* Store a 4-byte word in big-endian (unsigned long) */
        buf.putInt(off, (int) (val & 0xFFFFFFFFL));
    }

    /* ByteBuffer — little-endian helpers (for defendian.h LITT_ENDIAN context) */

    public static int ld_16le(byte[] buf, int off)
    {
        /* Load a 2-byte little-endian word — mirrors FatFs ld_16 */
        return (buf[off] & 0xFF) | ((buf[off + 1] & 0xFF) << 8);
    }

    public static long ld_32le(byte[] buf, int off)
    {
        /* Load a 4-byte little-endian word — mirrors FatFs ld_32 */
        return (buf[off] & 0xFFL) | ((buf[off + 1] & 0xFFL) << 8) | ((buf[off + 2] & 0xFFL) << 16) |
                ((buf[off + 3] & 0xFFL) << 24);
    }

    public static void st_16le(byte[] buf, int off, int val)
    {
        /* Store a 2-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
    }

    public static void st_32le(byte[] buf, int off, long val)
    {
        /* Store a 4-byte word in little-endian */
        buf[off] = (byte) (val & 0xFF);
        buf[off + 1] = (byte) ((val >> 8) & 0xFF);
        buf[off + 2] = (byte) ((val >> 16) & 0xFF);
        buf[off + 3] = (byte) ((val >> 24) & 0xFF);
    }

    public static int ld_16le(ByteBuffer buf, int off)
    {
        /* little-endian 16-bit from ByteBuffer */
        int b0 = buf.get(off) & 0xFF;
        int b1 = buf.get(off + 1) & 0xFF;
        return b0 | (b1 << 8);
    }

    public static long ld_32le(ByteBuffer buf, int off)
    {
        /* little-endian 32-bit from ByteBuffer */
        long b0 = buf.get(off) & 0xFFL;
        long b1 = buf.get(off + 1) & 0xFFL;
        long b2 = buf.get(off + 2) & 0xFFL;
        long b3 = buf.get(off + 3) & 0xFFL;
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    public static void st_16le(ByteBuffer buf, int off, int val)
    {
        /* little-endian 16-bit to ByteBuffer */
        buf.put(off, (byte) (val & 0xFF));
        buf.put(off + 1, (byte) ((val >> 8) & 0xFF));
    }

    public static void st_32le(ByteBuffer buf, int off, long val)
    {
        /* little-endian 32-bit to ByteBuffer */
        buf.put(off, (byte) (val & 0xFF));
        buf.put(off + 1, (byte) ((val >> 8) & 0xFF));
        buf.put(off + 2, (byte) ((val >> 16) & 0xFF));
        buf.put(off + 3, (byte) ((val >> 24) & 0xFF));
    }

    /* ByteBuffer bulk copy helpers — without touching position/limit */

    /**
     * Copies {@code len} bytes from {@code src} at {@code srcOff} into {@code buf}
     * at absolute offset {@code dstOff} without changing position/limit.
     */
    public static void copyToBuffer(byte[] src, int srcOff, ByteBuffer buf, int dstOff, int len)
    {
        if (buf.hasArray())
        {
            System.arraycopy(src, srcOff, buf.array(), buf.arrayOffset() + dstOff, len);
        } else
        {
            for (int i = 0; i < len; i++)
            {
                buf.put(dstOff + i, src[srcOff + i]);
            }
        }
    }

    /**
     * Copies {@code len} bytes from {@code buf} at absolute offset {@code srcOff}
     * into {@code dst} at {@code dstOff} without changing position/limit.
     */
    public static void copyFromBuffer(ByteBuffer buf, int srcOff, byte[] dst, int dstOff, int len)
    {
        if (buf.hasArray())
        {
            System.arraycopy(buf.array(), buf.arrayOffset() + srcOff, dst, dstOff, len);
        } else
        {
            for (int i = 0; i < len; i++)
            {
                dst[dstOff + i] = buf.get(srcOff + i);
            }
        }
    }

    /**
     * Ensures the ByteBuffer is in big-endian order for ADF block I/O.
     * Returns the same buffer for chaining.
     */
    public static ByteBuffer asBigEndian(ByteBuffer buf)
    {
        buf.order(ByteOrder.BIG_ENDIAN);
        return buf;
    }

    /**
     * Ensures the ByteBuffer is in little-endian order.
     * Returns the same buffer for chaining.
     */
    public static ByteBuffer asLittleEndian(ByteBuffer buf)
    {
        buf.order(ByteOrder.LITTLE_ENDIAN);
        return buf;
    }
}
