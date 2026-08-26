/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct DateTime
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

package com.github.adflib;


/**
 * {@code struct DateTime}.
 *
 * <pre>
 * struct DateTime
 * {
 *     int year, mon, day, hour, min, sec;
 * };
 * </pre>
 */
public final class DateTime {

    public int year;
    public int mon;
    public int day;
    public int hour;
    public int min;
    public int sec;

    public DateTime() {
    }

    public DateTime(int year, int mon, int day, int hour, int min, int sec) {
        this.year = year;
        this.mon = mon;
        this.day = day;
        this.hour = hour;
        this.min = min;
        this.sec = sec;
    }
}
