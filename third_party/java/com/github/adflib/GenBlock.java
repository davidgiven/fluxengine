/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct GenBlock
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
 * {@code struct GenBlock}.
 *
 * <pre>
 * struct GenBlock
 * {
 *     SECTNUM sect;
 *     SECTNUM parent;
 *     int type;
 *     int secType;
 *     char* name; // if (type == 2 and (secType==2 or secType==-3))
 * };
 * </pre>
 */
public final class GenBlock {

    public int sect;
    public int parent;
    public int type;
    public int secType;
    public String name; /* if (type == 2 and (secType==2 or secType==-3)) */

    public GenBlock() {
    }
}
