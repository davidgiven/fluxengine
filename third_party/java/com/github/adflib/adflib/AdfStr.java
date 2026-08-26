/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h
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
 * Java counterpart of {@code adf_str.h} — aggregator / documentation hub.
 *
 * <p>{@code adf_str.h} is the central structure header of ADFLib. Its
 * individual {@code struct} definitions are mapped to dedicated POJOs in
 * this package:
 * <ul>
 *   <li>{@link Volume} — {@code struct Volume}</li>
 *   <li>{@link Partition} — {@code struct Partition}</li>
 *   <li>{@link Device} — {@code struct Device}</li>
 *   <li>{@link File} — {@code struct File}</li>
 *   <li>{@link Entry} — {@code struct Entry}</li>
 *   <li>{@link CacheEntry} — {@code struct CacheEntry}</li>
 *   <li>{@link DateTime} — {@code struct DateTime}</li>
 *   <li>{@link Env} — {@code struct Env}</li>
 *   <li>{@link AdfList} — {@code struct List}</li>
 *   <li>{@link GenBlock} — {@code struct GenBlock}</li>
 *   <li>{@link FileBlocks} — {@code struct FileBlocks}</li>
 *   <li>{@link BEntryBlock} — {@code struct bEntryBlock}</li>
 * </ul>
 *
 * <p>Device-type constants ({@code DEVTYPE_*}) and environment selectors
 * ({@code PR_*}) live in {@link AdfConstants}. Return codes live in
 * {@link AdfError}.
 *
 * <p>The C macro {@code ENV_DECLARATION} ({@code struct Env adfEnv}) has no
 * Java equivalent; create an {@link Env} instance explicitly.
 *
 * <p>No string-handling functions are defined in {@code adf_str.h}; this class
 * exists only as a translation marker so the header has a 1:1 Java file.
 */
public final class AdfStr
{

    private AdfStr()
    {
    }

    /**
     * Placeholder mirroring the C {@code ENV_DECLARATION} macro.
     * Create your own {@link Env} instance instead of using a global.
     */
    public static Env createEnv()
    {
        return new Env();
    }
}
