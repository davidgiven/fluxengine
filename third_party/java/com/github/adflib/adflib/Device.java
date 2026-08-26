/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct Device
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

import java.nio.ByteBuffer;
import java.util.ArrayList;
import java.util.List;

/**
 * {@code struct Device} — now abstract, dump path removed.
 *
 * <pre>
 * struct Device
 * {
 *     int devType; // see below
 *     BOOL readOnly;
 *     int32_t size; // in bytes
 *     int nVol; // partitions
 *     struct Volume** volList;
 *     int32_t cylinders; // geometry
 *     int32_t heads;
 *     int32_t sectors;
 * };
 * </pre>
 * <p>
 * Construction does all init; callers create a concrete subclass implementing
 * sector I/O and pass the instance to {@code AdfHd.adfMountDev(Device)}.
 */
public abstract class Device
{

    public int devType; /* see below */
    public boolean readOnly;
    public int size; /* in bytes */

    public int nVol; /* partitions */
    public List<Volume> volList = new ArrayList<>();

    public int cylinders; /* geometry */
    public int heads;
    public int sectors;

    public Device()
    {
    }

    /* Sector I/O — replaces nativeFunctions table; ByteBuffer absolute put/get without touching
    position/limit */

    public abstract AdfError adfReadSector(int n, int size, ByteBuffer buf);

    public AdfError adfReadSector(int n, int size, byte[] buf)
    {
        return adfReadSector(n, size, ByteBuffer.wrap(buf).order(java.nio.ByteOrder.BIG_ENDIAN));
    }

    public abstract AdfError adfWriteSector(int n, int size, ByteBuffer buf);

    public AdfError adfWriteSector(int n, int size, byte[] buf)
    {
        return adfWriteSector(n, size, ByteBuffer.wrap(buf).order(java.nio.ByteOrder.BIG_ENDIAN));
    }

    public abstract AdfError adfReleaseDevice();
}
