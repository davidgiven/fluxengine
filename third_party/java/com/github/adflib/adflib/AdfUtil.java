/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_util.c / adf_util.h
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
import java.util.Calendar;

/**
 * Java port of {@code adf_util.c} / {@code adf_util.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data.
 * High-level objects use normal Java classes ({@link DateTime}, {@link AdfList}).
 * Return codes use {@link AdfError} with out-parameters as single-element arrays.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfUtil
{

    private AdfUtil()
    {
    }

    /**
     * Global environment — mirrors {@code extern struct Env adfEnv}.
     */
    public static Env adfEnv = AdfRaw.adfEnv;

    /*
     * swLong
     *
     * write an uint32_t value (val) (in)
     * to an uint8_t* buffer (buf) (out)
     *
     * used in adfWrite----Block() functions
     */

    public static void swLong(byte[] buf, int off, long val)
    {
        buf[off] = (byte) ((val & 0xff000000L) >> 24);
        buf[off + 1] = (byte) ((val & 0x00ff0000L) >> 16);
        buf[off + 2] = (byte) ((val & 0x0000ff00L) >> 8);
        buf[off + 3] = (byte) (val & 0x000000ffL);
    }

    public static void swLong(ByteBuffer buf, int off, long val)
    {
        buf.put(off, (byte) ((val & 0xff000000L) >> 24));
        buf.put(off + 1, (byte) ((val & 0x00ff0000L) >> 16));
        buf.put(off + 2, (byte) ((val & 0x0000ff00L) >> 8));
        buf.put(off + 3, (byte) (val & 0x000000ffL));
    }

    public static void swShort(byte[] buf, int off, int val)
    {
        buf[off] = (byte) ((val & 0xff00) >> 8);
        buf[off + 1] = (byte) (val & 0x00ff);
    }

    public static void swShort(ByteBuffer buf, int off, int val)
    {
        buf.put(off, (byte) ((val & 0xff00) >> 8));
        buf.put(off + 1, (byte) (val & 0x00ff));
    }

    /*
     * newCell
     *
     * adds a cell at the end the list
     */

    public static AdfList newCell(AdfList list, Object content)
    {
        AdfList cell = new AdfList();
        if (cell == null)
        {
            if (adfEnv != null && adfEnv.eFct != null)
            {
                adfEnv.eFct.call("newCell : malloc");
            }
            return null;
        }
        cell.content = content;
        cell.next = null;
        cell.subdir = null;
        if (list != null)
        {
            list.next = cell;
        }

        return cell;
    }

    /*
     * freeList
     *
     */

    public static void freeList(AdfList list)
    {
        if (list == null)
        {
            return;
        }

        if (list.next != null)
        {
            freeList(list.next);
        }
        /* free(list); handled by GC */
    }

    /*
     * Days2Date
     *
     * amiga disk date format (days) to normal dd/mm/yy format (out)
     */

    public static void adfDays2Date(int days, int[] yy, int[] mm, int[] dd)
    {
        int y = 0;
        int m = 0;
        int nd = 0;
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        /* 0 = 1 Jan 1978,  6988 = 18 feb 1997 */

        /*--- year ---*/
        y = 1978;
        if (adfIsLeap(y))
        {
            nd = 366;
        } else
        {
            nd = 365;
        }
        while (days >= nd)
        {
            days -= nd;
            y++;
            if (adfIsLeap(y))
            {
                nd = 366;
            } else
            {
                nd = 365;
            }
        }

        /*--- month ---*/
        m = 1;
        if (adfIsLeap(y))
        {
            jm[2 - 1] = 29;
        }
        while (days >= jm[m - 1])
        {
            days -= jm[m - 1];
            m++;
        }

        yy[0] = y;
        mm[0] = m;
        dd[0] = days + 1;
    }

    /*
     * IsLeap
     *
     * true if a year (y) is leap
     */

    public static boolean adfIsLeap(int y)
    {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
    }

    /*
     * adfCurrentDateTime
     *
     * return the current system date and time
     */

    public static DateTime adfGiveCurrentTime()
    {
        Calendar cal = Calendar.getInstance();
        DateTime r = new DateTime();

        r.year = cal.get(Calendar.YEAR) - 1900;         /* since 1900 */
        r.mon = cal.get(Calendar.MONTH) + 1;
        r.day = cal.get(Calendar.DAY_OF_MONTH);
        r.hour = cal.get(Calendar.HOUR_OF_DAY);
        r.min = cal.get(Calendar.MINUTE);
        r.sec = cal.get(Calendar.SECOND);

        return r;
    }

    /*
     * adfTime2AmigaTime
     *
     * converts date and time (dt) into Amiga format : day, min, ticks
     */

    public static void adfTime2AmigaTime(DateTime dt, int[] day, int[] min, int[] ticks)
    {
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        min[0] = dt.hour * 60 + dt.min;                /* mins */
        ticks[0] = dt.sec * 50;                        /* ticks */

        /*--- days ---*/

        day[0] = dt.day - 1;                         /* current month days */

        /* previous months days downto january */
        if (dt.mon > 1)
        {                      /* if previous month exists */
            int mon = dt.mon - 1;
            if (mon > 2 && adfIsLeap(dt.year))
            {    /* months after a leap february */
                jm[2 - 1] = 29;
            }
            while (mon > 0)
            {
                day[0] = day[0] + jm[mon - 1];
                mon--;
            }
        }

        /* years days before current year downto 1978 */
        if (dt.year > 78)
        {
            int year = dt.year - 1;
            while (year >= 78)
            {
                if (adfIsLeap(year))
                {
                    day[0] = day[0] + 366;
                } else
                {
                    day[0] = day[0] + 365;
                }
                year--;
            }
        }
    }

    /*
     * dumpBlock
     *
     * debug function : to dump a block before writing the check its contents
     *
     */

    public static void dumpBlock(byte[] buf)
    {
        int i = 0;
        int j = 0;

        for (i = 0; i < 32; i++)
        {
            System.out.printf("%5x ", i * 16);
            for (j = 0; j < 4; j++)
            {
                System.out.printf("%08x ", AdfEndian.Long(buf, j * 4 + i * 16));
            }
            System.out.printf("    ");
            for (j = 0; j < 16; j++)
            {
                int c = buf[i * 16 + j] & 0xFF;
                if (c < 32 || c > 127)
                {
                    System.out.printf(".");
                } else
                {
                    System.out.printf("%c", (char) c);
                }
            }
            System.out.printf("\n");
        }
    }

    public static void dumpBlock(ByteBuffer buf)
    {
        int i = 0;
        int j = 0;

        for (i = 0; i < 32; i++)
        {
            System.out.printf("%5x ", i * 16);
            for (j = 0; j < 4; j++)
            {
                int off = j * 4 + i * 16;
                long v = ((buf.get(off) & 0xFFL) << 24) | ((buf.get(off + 1) & 0xFFL) << 16) |
                        ((buf.get(off + 2) & 0xFFL) << 8) | (buf.get(off + 3) & 0xFFL);
                System.out.printf("%08x ", v);
            }
            System.out.printf("    ");
            for (j = 0; j < 16; j++)
            {
                int c = buf.get(i * 16 + j) & 0xFF;
                if (c < 32 || c > 127)
                {
                    System.out.printf(".");
                } else
                {
                    System.out.printf("%c", (char) c);
                }
            }
            System.out.printf("\n");
        }
    }
}
