/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  hd_blk.h — bBADBentry
 *
 *  $Id$
 *
 *  hard disk blocks structures
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

import java.nio.ByteBuffer;
import java.nio.ByteOrder;


/**
 * {@code struct bBADBentry}.
 *
 * <pre>
 * struct bBADBentry
 * {
 *     int32_t badBlock;
 *     int32_t goodBlock;
 * };
 * </pre>
 */
public final class BBADBEntry {

    /*000*/ public int badBlock;
    /*004*/ public int goodBlock;

    public BBADBEntry() {
    }

    public BBADBEntry(int badBlock, int goodBlock) {
        this.badBlock = badBlock;
        this.goodBlock = goodBlock;
    }

    public static BBADBEntry read(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        BBADBEntry e = new BBADBEntry();
        e.badBlock = b.getInt(off + 0);
        e.goodBlock = b.getInt(off + 4);
        return e;
    }

    public void write(ByteBuffer buf, int off) {
        ByteBuffer b = buf.duplicate().order(ByteOrder.BIG_ENDIAN);
        b.putInt(off + 0, badBlock);
        b.putInt(off + 4, goodBlock);
    }

    public static final int SIZE = 8;
}
