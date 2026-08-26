/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  prefix.h
 *
 *  $Id$
 *
 *  adds symbol export directive under windows
 *  does nothing under Linux
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
 * Java counterpart of {@code prefix.h}.
 *
 * <p>In C:
 * <pre>
 *   #ifdef WIN32DLL
 *   #define PREFIX __declspec(dllexport)
 *   #else
 *   #define PREFIX
 *   #endif
 * </pre>
 * In Java no export macro is needed; this class documents the decision.
 */
public final class AdfPrefix {

    private AdfPrefix() {
    }

    /** No-op marker — {@code PREFIX} is empty outside WIN32DLL builds. */
    public static final String PREFIX = "";
}
