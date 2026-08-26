/*
 * ADF Library
 *
 * adf_link.c / adf_link.h
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

package com.github.adflib;

/**
 * Java port of {@code adf_link.c} / {@code adf_link.h}.
 *
 * <p>Keeps original C control flow and helper naming. High-level objects use
 * normal Java classes ({@link Entry}). Return codes use {@link AdfError} with
 * out-parameters as single-element arrays.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfLink {

    private AdfLink() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    /*
     *
     *
     */

    public static AdfError adfBlockPtr2EntryName(Volume vol, int nSect, int lPar,
            String[] name, int[] size) {
        BEntryBlock entryBlk = new BEntryBlock();
        Entry entry = new Entry();

        if (name[0] == null) {
            AdfDir.adfReadEntryBlock(vol, nSect, entryBlk);
            size[0] = entryBlk.byteSize;
            return AdfError.RC_OK;
            /*        adfEntBlock2Entry(&entryBlk, &entry);	/*error*/
            /*        if (entryBlk.secType!=ST_ROOT && entry.parent!=lPar)
                printf("path=%s\n",path(vol,entry.parent));
            */
            /*        *name = strdup("");
            if (*name==NULL)
                return RC_MALLOC;
            return RC_OK;
            */
        } else {
            return AdfError.RC_OK;
        }
    }
}
