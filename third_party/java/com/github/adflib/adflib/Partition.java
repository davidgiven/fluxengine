/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct Partition
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
 * {@code struct Partition}.
 *
 * <pre>
 * struct Partition
 * {
 *     int32_t startCyl;
 *     int32_t lenCyl;
 *     char* volName;
 *     int volType;
 * };
 * </pre>
 */
public final class Partition
{

    public int startCyl;
    public int lenCyl;
    public String volName;
    public int volType;

    public Partition()
    {
    }

    public Partition(int startCyl, int lenCyl, String volName, int volType)
    {
        this.startCyl = startCyl;
        this.lenCyl = lenCyl;
        this.volName = volName;
        this.volType = volType;
    }
}
