/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct CacheEntry
 *
 *  $Id$
 *
 *  structures/constants definitions
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


/**
 * {@code struct CacheEntry}.
 *
 * <pre>
 * struct CacheEntry
 * {
 *     int32_t header, size, protect;
 *     short days, mins, ticks;
 *     signed char type;
 *     char nLen, cLen;
 *     char name[MAXNAMELEN + 1], comm[MAXCMMTLEN + 1];
 * };
 * </pre>
 */
public final class CacheEntry
{

    public int header;
    public int size;
    public int protect;
    public short days;
    public short mins;
    public short ticks;
    public byte type; /* signed char */
    public byte nLen;
    public byte cLen;
    public byte[] name = new byte[AdfConstants.MAXNAMELEN + 1];
    public byte[] comm = new byte[AdfConstants.MAXCMMTLEN + 1];

    public CacheEntry()
    {
    }
}
