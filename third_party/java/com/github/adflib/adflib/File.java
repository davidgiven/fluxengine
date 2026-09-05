/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct File
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
 * {@code struct File} — open file handle.
 *
 * <pre>
 * struct File
 * {
 *     struct Volume* volume;
 *     struct bFileHeaderBlock* fileHdr;
 *     void* currentData;
 *     struct bFileExtBlock* currentExt;
 *     int32_t nDataBlock;
 *     SECTNUM curDataPtr;
 *     uint32_t pos;
 *     int posInDataBlk;
 *     int posInExtBlk;
 *     BOOL eof, writeMode;
 * };
 * </pre>
 */
public final class File
{

    public Volume volume;

    public BFileHeaderBlock fileHdr;
    public java.nio.ByteBuffer currentData;
    public BFileExtBlock currentExt;

    public int nDataBlock;
    public int curDataPtr;
    public long pos; /* uint32_t */

    public int posInDataBlk;
    public int posInExtBlk;
    public boolean eof;
    public boolean writeMode;

    public File()
    {
    }
}
