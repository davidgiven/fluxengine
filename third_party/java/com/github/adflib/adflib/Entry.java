/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct Entry
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
 * {@code struct Entry} — directory entry.
 *
 * <pre>
 * struct Entry
 * {
 *     int type;
 *     char* name;
 *     SECTNUM sector;
 *     SECTNUM real;
 *     SECTNUM parent;
 *     char* comment;
 *     uint32_t size;
 *     int32_t access;
 *     int year, month, days;
 *     int hour, mins, secs;
 * };
 * </pre>
 */
public final class Entry
{

    public int type;
    public String name;
    public int sector;
    public int real;
    public int parent;
    public String comment;
    public long size; /* uint32_t */
    public int access;
    public int year;
    public int month;
    public int days;
    public int hour;
    public int mins;
    public int secs;

    public Entry()
    {
    }
}
