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
 * $Id: data.c,v 1.7 1998/11/02 22:08:57 rob Exp $
 */

package org.mars.hfsutils;

public final class HfsData
{
    private HfsData()
    {
    }

    /*
     * TIMEDIFF — seconds between MacOS epoch (Jan 1 1904) and Unix epoch (Jan 1 1970)
     */
    public static final long TIMEDIFF = 2082844800L;

    /*
     * timezone difference between local time and UTC (seconds);
     * initialised to -1 as a sentinel; computed on first use
     */
    private static long tzdiff = -1;

    /*
     * hfs_charorder — MacRoman collation order (256 bytes)
     */
    public static final int[] HFS_CHARORDER =
    {
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
        0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,

        0x20, 0x22, 0x23, 0x28, 0x29, 0x2a, 0x2b, 0x2c,
        0x2f, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36,
        0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e,
        0x3f, 0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46,

        0x47, 0x48, 0x58, 0x5a, 0x5e, 0x60, 0x67, 0x69,
        0x6b, 0x6d, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7f,
        0x8d, 0x8f, 0x91, 0x93, 0x96, 0x98, 0x9f, 0xa1,
        0xa3, 0xa5, 0xa8, 0xaa, 0xab, 0xac, 0xad, 0xae,

        0x54, 0x48, 0x58, 0x5a, 0x5e, 0x60, 0x67, 0x69,
        0x6b, 0x6d, 0x73, 0x75, 0x77, 0x79, 0x7b, 0x7f,
        0x8d, 0x8f, 0x91, 0x93, 0x96, 0x98, 0x9f, 0xa1,
        0xa3, 0xa5, 0xa8, 0xaf, 0xb0, 0xb1, 0xb2, 0xb3,

        0x4c, 0x50, 0x5c, 0x62, 0x7d, 0x81, 0x9a, 0x55,
        0x4a, 0x56, 0x4c, 0x4e, 0x50, 0x5c, 0x62, 0x64,
        0x65, 0x66, 0x6f, 0x70, 0x71, 0x72, 0x7d, 0x89,
        0x8a, 0x8b, 0x81, 0x83, 0x9c, 0x9d, 0x9e, 0x9a,

        0xb4, 0xb5, 0xb6, 0xb7, 0xb8, 0xb9, 0xba, 0x95,
        0xbb, 0xbc, 0xbd, 0xbe, 0xbf, 0xc0, 0x52, 0x85,
        0xc1, 0xc2, 0xc3, 0xc4, 0xc5, 0xc6, 0xc7, 0xc8,
        0xc9, 0xca, 0xcb, 0x57, 0x8c, 0xcc, 0x52, 0x85,

        0xcd, 0xce, 0xcf, 0xd0, 0xd1, 0xd2, 0xd3, 0x26,
        0x27, 0xd4, 0x20, 0x4a, 0x4e, 0x83, 0x87, 0x87,
        0xd5, 0xd6, 0x24, 0x25, 0x2d, 0x2e, 0xd7, 0xd8,
        0xa7, 0xd9, 0xda, 0xdb, 0xdc, 0xdd, 0xde, 0xdf,

        0xe0, 0xe1, 0xe2, 0xe3, 0xe4, 0xe5, 0xe6, 0xe7,
        0xe8, 0xe9, 0xea, 0xeb, 0xec, 0xed, 0xee, 0xef,
        0xf0, 0xf1, 0xf2, 0xf3, 0xf4, 0xf5, 0xf6, 0xf7,
        0xf8, 0xf9, 0xfa, 0xfb, 0xfc, 0xfd, 0xfe, 0xff
    };

    /*
     * NAME:	data->getsb()
     * DESCRIPTION:	marshal 1 signed byte into local host format
     */
    public static byte d_getsb(byte[] arr, int offset)
    {
        return arr[offset];
    }

    /*
     * NAME:	data->getub()
     * DESCRIPTION:	marshal 1 unsigned byte into local host format
     */
    public static int d_getub(byte[] arr, int offset)
    {
        return arr[offset] & 0xff;
    }

    /*
     * NAME:	data->getsw()
     * DESCRIPTION:	marshal 2 signed bytes into local host format
     */
    public static short d_getsw(byte[] arr, int offset)
    {
        return (short) (((arr[offset] & 0xff) << 8) |
                        ((arr[offset + 1] & 0xff)));
    }

    /*
     * NAME:	data->getuw()
     * DESCRIPTION:	marshal 2 unsigned bytes into local host format
     */
    public static int d_getuw(byte[] arr, int offset)
    {
        return (((arr[offset] & 0xff) << 8) |
                ((arr[offset + 1] & 0xff)));
    }

    /*
     * NAME:	data->getsl()
     * DESCRIPTION:	marshal 4 signed bytes into local host format
     */
    public static long d_getsl(byte[] arr, int offset)
    {
        return ((long) (arr[offset] & 0xff) << 24) |
               ((long) (arr[offset + 1] & 0xff) << 16) |
               ((long) (arr[offset + 2] & 0xff) << 8) |
               ((long) (arr[offset + 3] & 0xff));
    }

    /*
     * NAME:	data->getul()
     * DESCRIPTION:	marshal 4 unsigned bytes into local host format
     */
    public static long d_getul(byte[] arr, int offset)
    {
        return ((long) (arr[offset] & 0xff) << 24) |
               ((long) (arr[offset + 1] & 0xff) << 16) |
               ((long) (arr[offset + 2] & 0xff) << 8) |
               ((long) (arr[offset + 3] & 0xff));
    }

    /*
     * NAME:	data->putsb()
     * DESCRIPTION:	marshal 1 signed byte out in big-endian format
     */
    public static void d_putsb(byte[] arr, int offset, byte data)
    {
        arr[offset] = data;
    }

    /*
     * NAME:	data->putub()
     * DESCRIPTION:	marshal 1 unsigned byte out in big-endian format
     */
    public static void d_putub(byte[] arr, int offset, int data)
    {
        arr[offset] = (byte) data;
    }

    /*
     * NAME:	data->putsw()
     * DESCRIPTION:	marshal 2 signed bytes out in big-endian format
     */
    public static void d_putsw(byte[] arr, int offset, short data)
    {
        arr[offset] = (byte) (((data & 0xff00) >> 8) & 0xff);
        arr[offset + 1] = (byte) ((data & 0x00ff) & 0xff);
    }

    /*
     * NAME:	data->putuw()
     * DESCRIPTION:	marshal 2 unsigned bytes out in big-endian format
     */
    public static void d_putuw(byte[] arr, int offset, int data)
    {
        arr[offset] = (byte) ((data & 0xff00) >> 8);
        arr[offset + 1] = (byte) ((data & 0x00ff) >> 0);
    }

    /*
     * NAME:	data->putsl()
     * DESCRIPTION:	marshal 4 signed bytes out in big-endian format
     */
    public static void d_putsl(byte[] arr, int offset, long data)
    {
        arr[offset] = (byte) ((data & 0xff000000L) >> 24);
        arr[offset + 1] = (byte) ((data & 0x00ff0000L) >> 16);
        arr[offset + 2] = (byte) ((data & 0x0000ff00L) >> 8);
        arr[offset + 3] = (byte) ((data & 0x000000ffL) >> 0);
    }

    /*
     * NAME:	data->putul()
     * DESCRIPTION:	marshal 4 unsigned bytes out in big-endian format
     */
    public static void d_putul(byte[] arr, int offset, long data)
    {
        arr[offset] = (byte) ((data & 0xff000000L) >> 24);
        arr[offset + 1] = (byte) ((data & 0x00ff0000L) >> 16);
        arr[offset + 2] = (byte) ((data & 0x0000ff00L) >> 8);
        arr[offset + 3] = (byte) ((data & 0x000000ffL) >> 0);
    }

    /*
     * NAME:	data->fetchsb()
     * DESCRIPTION:	incrementally retrieve a signed byte of data
     */
    public static byte d_fetchsb(byte[] arr, int[] cursor)
    {
        byte val = arr[cursor[0]];
        cursor[0] += 1;
        return val;
    }

    /*
     * NAME:	data->fetchub()
     * DESCRIPTION:	incrementally retrieve an unsigned byte of data
     */
    public static int d_fetchub(byte[] arr, int[] cursor)
    {
        int val = arr[cursor[0]] & 0xff;
        cursor[0] += 1;
        return val;
    }

    /*
     * NAME:	data->fetchsw()
     * DESCRIPTION:	incrementally retrieve a signed word of data
     */
    public static short d_fetchsw(byte[] arr, int[] cursor)
    {
        short val = (short) (((arr[cursor[0]] & 0xff) << 8) |
                             ((arr[cursor[0] + 1] & 0xff)));
        cursor[0] += 2;
        return val;
    }

    /*
     * NAME:	data->fetchuw()
     * DESCRIPTION:	incrementally retrieve an unsigned word of data
     */
    public static int d_fetchuw(byte[] arr, int[] cursor)
    {
        int val = ((arr[cursor[0]] & 0xff) << 8) |
                  ((arr[cursor[0] + 1] & 0xff));
        cursor[0] += 2;
        return val;
    }

    /*
     * NAME:	data->fetchsl()
     * DESCRIPTION:	incrementally retrieve a signed long word of data
     */
    public static long d_fetchsl(byte[] arr, int[] cursor)
    {
        long val = ((long) (arr[cursor[0]] & 0xff) << 24) |
                   ((long) (arr[cursor[0] + 1] & 0xff) << 16) |
                   ((long) (arr[cursor[0] + 2] & 0xff) << 8) |
                   ((long) (arr[cursor[0] + 3] & 0xff));
        cursor[0] += 4;
        return val;
    }

    /*
     * NAME:	data->fetchul()
     * DESCRIPTION:	incrementally retrieve an unsigned long word of data
     */
    public static long d_fetchul(byte[] arr, int[] cursor)
    {
        long val = ((long) (arr[cursor[0]] & 0xff) << 24) |
                   ((long) (arr[cursor[0] + 1] & 0xff) << 16) |
                   ((long) (arr[cursor[0] + 2] & 0xff) << 8) |
                   ((long) (arr[cursor[0] + 3] & 0xff));
        cursor[0] += 4;
        return val;
    }

    /*
     * NAME:	data->storesb()
     * DESCRIPTION:	incrementally store a signed byte of data
     */
    public static void d_storesb(byte[] arr, int[] cursor, byte data)
    {
        arr[cursor[0]] = data;
        cursor[0] += 1;
    }

    /*
     * NAME:	data->storeub()
     * DESCRIPTION:	incrementally store an unsigned byte of data
     */
    public static void d_storeub(byte[] arr, int[] cursor, int data)
    {
        arr[cursor[0]] = (byte) data;
        cursor[0] += 1;
    }

    /*
     * NAME:	data->storesw()
     * DESCRIPTION:	incrementally store a signed word of data
     */
    public static void d_storesw(byte[] arr, int[] cursor, short data)
    {
        arr[cursor[0]] = (byte) (((data & 0xff00) >> 8) & 0xff);
        arr[cursor[0] + 1] = (byte) ((data & 0x00ff) & 0xff);
        cursor[0] += 2;
    }

    /*
     * NAME:	data->storeuw()
     * DESCRIPTION:	incrementally store an unsigned word of data
     */
    public static void d_storeuw(byte[] arr, int[] cursor, int data)
    {
        arr[cursor[0]] = (byte) ((data & 0xff00) >> 8);
        arr[cursor[0] + 1] = (byte) ((data & 0x00ff) >> 0);
        cursor[0] += 2;
    }

    /*
     * NAME:	data->storesl()
     * DESCRIPTION:	incrementally store a signed long word of data
     */
    public static void d_storesl(byte[] arr, int[] cursor, long data)
    {
        arr[cursor[0]] = (byte) ((data & 0xff000000L) >> 24);
        arr[cursor[0] + 1] = (byte) ((data & 0x00ff0000L) >> 16);
        arr[cursor[0] + 2] = (byte) ((data & 0x0000ff00L) >> 8);
        arr[cursor[0] + 3] = (byte) ((data & 0x000000ffL) >> 0);
        cursor[0] += 4;
    }

    /*
     * NAME:	data->storeul()
     * DESCRIPTION:	incrementally store an unsigned long word of data
     */
    public static void d_storeul(byte[] arr, int[] cursor, long data)
    {
        arr[cursor[0]] = (byte) ((data & 0xff000000L) >> 24);
        arr[cursor[0] + 1] = (byte) ((data & 0x00ff0000L) >> 16);
        arr[cursor[0] + 2] = (byte) ((data & 0x0000ff00L) >> 8);
        arr[cursor[0] + 3] = (byte) ((data & 0x000000ffL) >> 0);
        cursor[0] += 4;
    }

    /*
     * NAME:	data->fetchstr()
     * DESCRIPTION:	incrementally retrieve a string
     */
    public static void d_fetchstr(byte[] arr, int[] cursor, char[] dest, int size)
    {
        int len = d_getub(arr, cursor[0]);

        if (len > 0 && len < size)
        {
            for (int i = 0; i < len; i++)
                dest[i] = (char) (arr[cursor[0] + 1 + i] & 0xff);
        }
        else
        {
            len = 0;
        }

        dest[len] = 0;
        cursor[0] += size;
    }

    /*
     * NAME:	data->storestr()
     * DESCRIPTION:	incrementally store a string
     */
    public static void d_storestr(byte[] arr, int[] cursor, char[] src, int size)
    {
        int len = strlen(src);
        if (len > size - 1)
            len = 0;

        d_storeub(arr, cursor, (byte) len);

        int srcOff = 0;
        int destOff = cursor[0];
        for (int i = 0; i < len; i++)
            arr[destOff + i] = (byte) src[srcOff++];
        for (int i = len; i < size - 1; i++)
            arr[destOff + i] = 0;

        cursor[0] += size - 1;
    }

    /*
     * NAME:	data->relstring()
     * DESCRIPTION:	compare two strings as per MacOS for HFS
     */
    public static int d_relstring(char[] str1, char[] str2)
    {
        int i = 0;

        while (i < str1.length && str1[i] != 0 &&
               i < str2.length && str2[i] != 0)
        {
            int diff = (HFS_CHARORDER[str1[i] & 0xff] & 0xff) -
                       (HFS_CHARORDER[str2[i] & 0xff] & 0xff);

            if (diff != 0)
                return diff;

            i++;
        }

        if (i < str1.length && str1[i] != 0)
            return 1;
        else if (i < str2.length && str2[i] != 0)
            return -1;

        return 0;
    }

    /*
     * NAME:	calctzdiff()
     * DESCRIPTION:	calculate the timezone difference between local time and UTC
     */
    private static void calctzdiff()
    {
        java.time.Instant now = java.time.Instant.now();
        java.time.ZoneOffset localOffset =
            java.time.ZoneId.systemDefault().getRules().getOffset(now);
        tzdiff = localOffset.getTotalSeconds();
    }

    /*
     * NAME:	data->ltime()
     * DESCRIPTION:	convert MacOS time to local time
     */
    public static long d_ltime(long mtime)
    {
        if (tzdiff == -1)
            calctzdiff();

        return (mtime - TIMEDIFF) - tzdiff;
    }

    /*
     * NAME:	data->mtime()
     * DESCRIPTION:	convert local time to MacOS time
     */
    public static long d_mtime(long ltime)
    {
        if (tzdiff == -1)
            calctzdiff();

        return (ltime + tzdiff) + TIMEDIFF;
    }

    /*
     * helper — return the length of a NUL-terminated char[]
     */
    private static int strlen(char[] s)
    {
        int i = 0;
        while (i < s.length && s[i] != 0)
            i++;
        return i;
    }
}
