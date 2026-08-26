/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct List
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
 * {@code struct List} — generic linked tree.
 *
 * <pre>
 * struct List
 * { // generic linked tree
 *     void* content;
 *     struct List* subdir;
 *     struct List* next;
 * };
 * </pre>
 *
 * Named {@code AdfList} to avoid collision with {@code java.util.List}.
 */
public final class AdfList {

    public Object content;
    public AdfList subdir;
    public AdfList next;

    public AdfList() {
    }

    public AdfList(Object content, AdfList subdir, AdfList next) {
        this.content = content;
        this.subdir = subdir;
        this.next = next;
    }
}
