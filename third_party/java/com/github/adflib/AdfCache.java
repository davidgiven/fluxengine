/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_cache.c / adf_cache.h
 *
 *  $Id$
 *
 *  directory cache code
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
import java.util.Arrays;

/**
 * Java port of {@code adf_cache.c} / {@code adf_cache.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data
 * via {@link AdfEndian}. High-level objects use normal Java classes
 * ({@link CacheEntry}, {@link BDirCacheBlock}, etc.). Return codes use
 * {@link AdfError} with out-parameters as single-element arrays.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfCache {

    private AdfCache() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    /*
     * adfGetDirEntCache
     *
     * replace 'adfGetDirEnt'. returns a the dir contents based on the dircache list
     */

    public static AdfList adfGetDirEntCache(Volume vol, int dir, boolean recurs) {
        BEntryBlock parent = new BEntryBlock();
        BDirCacheBlock dirc = new BDirCacheBlock();
        int offset = 0;
        int n = 0;
        AdfList cell = null;
        AdfList head = null;
        CacheEntry caEntry = new CacheEntry();
        Entry entry = null;
        int nSect = 0;

        if (adfReadEntryBlock(vol, dir, parent) != AdfError.RC_OK) {
            return null;
        }

        nSect = parent.extension;

        cell = null;
        head = null;
        do {
            /* one loop per cache block */
            n = 0;
            offset = 0;
            if (adfReadDirCBlock(vol, nSect, dirc) != AdfError.RC_OK) {
                return null;
            }
            while (n < dirc.recordsNb) {
                /* one loop per record */
                entry = new Entry();
                if (entry == null) {
                    adfFreeDirList(head);
                    return null;
                }
                int[] p = new int[]{offset};
                adfGetCacheEntry(dirc, p, caEntry);
                offset = p[0];

                /* converts a cache entry into a dir entry */
                entry.type = caEntry.type;
                // name
                int nLen = caEntry.nLen & 0xFF;
                byte[] nameBytes = new byte[nLen];
                System.arraycopy(caEntry.name, 0, nameBytes, 0, nLen);
                entry.name = new String(nameBytes);
                if (entry.name == null) {
                    adfFreeDirList(head);
                    return null;
                }
                entry.sector = caEntry.header;
                int cLen = caEntry.cLen & 0xFF;
                byte[] commBytes = new byte[cLen];
                if (cLen > 0) {
                    System.arraycopy(caEntry.comm, 0, commBytes, 0, cLen);
                }
                entry.comment = new String(commBytes);
                if (entry.comment == null) {
                    adfFreeDirList(head);
                    return null;
                }
                entry.size = caEntry.size & 0xFFFFFFFFL;
                entry.access = caEntry.protect;
                int[] yy = new int[1];
                int[] mm = new int[1];
                int[] dd = new int[1];
                adfDays2Date(caEntry.days & 0xFFFF, yy, mm, dd);
                entry.year = yy[0];
                entry.month = mm[0];
                entry.days = dd[0];
                entry.hour = (caEntry.mins & 0xFFFF) / 60;
                entry.mins = (caEntry.mins & 0xFFFF) % 60;
                entry.secs = (caEntry.ticks & 0xFFFF) / 50;

                /* add it into the linked list */
                if (head == null) {
                    head = newCell(null, entry);
                    cell = head;
                } else {
                    cell = newCell(cell, entry);
                }

                if (cell == null) {
                    adfFreeEntry(entry);
                    adfFreeDirList(head);
                    return null;
                }

                if (recurs && entry.type == AdfConstants.ST_DIR) {
                    cell.subdir = adfGetDirEntCache(vol, entry.sector, recurs);
                }

                n++;
            }
            nSect = dirc.nextDirC;
        } while (nSect != 0);

        return head;
    }

    /*
     * adfGetCacheEntry
     *
     * Returns a cache entry, starting from the offset p (the index into records[])
     * This offset is updated to the end of the returned entry.
     */

    public static void adfGetCacheEntry(BDirCacheBlock dirc, int[] p, CacheEntry cEntry) {
        int ptr = 0;

        ptr = p[0];

        /* LITT_ENDIAN handled via big-endian reads in Java */
        cEntry.header = AdfEndian.Long(dirc.records, ptr);
        cEntry.size = AdfEndian.Long(dirc.records, ptr + 4);
        cEntry.protect = AdfEndian.Long(dirc.records, ptr + 8);
        cEntry.days = (short) AdfEndian.Short(dirc.records, ptr + 16);
        cEntry.mins = (short) AdfEndian.Short(dirc.records, ptr + 18);
        cEntry.ticks = (short) AdfEndian.Short(dirc.records, ptr + 20);

        cEntry.type = (byte) dirc.records[ptr + 22];

        cEntry.nLen = dirc.records[ptr + 23];
        int nLen = cEntry.nLen & 0xFF;
        System.arraycopy(dirc.records, ptr + 24, cEntry.name, 0, nLen);
        cEntry.name[nLen] = 0;

        cEntry.cLen = dirc.records[ptr + 24 + nLen];
        int cLen = cEntry.cLen & 0xFF;
        if (cLen > 0) {
            System.arraycopy(dirc.records, ptr + 24 + nLen + 1, cEntry.comm, 0, cLen);
        }
        cEntry.comm[cLen] = 0;

        p[0] = ptr + 24 + nLen + 1 + cLen;

        /* the starting offset of each record must be even (68000 constraint) */
        if ((p[0] % 2) != 0) {
            p[0] = p[0] + 1;
        }
    }

    /*
     * adfPutCacheEntry
     *
     * remplaces one cache entry at the p offset, and returns its length
     */

    public static int adfPutCacheEntry(BDirCacheBlock dirc, int[] p, CacheEntry cEntry) {
        int ptr = 0;
        int l = 0;

        ptr = p[0];

        swLong(dirc.records, ptr, cEntry.header);
        swLong(dirc.records, ptr + 4, cEntry.size);
        swLong(dirc.records, ptr + 8, cEntry.protect);
        swShort(dirc.records, ptr + 16, cEntry.days & 0xFFFF);
        swShort(dirc.records, ptr + 18, cEntry.mins & 0xFFFF);
        swShort(dirc.records, ptr + 20, cEntry.ticks & 0xFFFF);

        dirc.records[ptr + 22] = (byte) cEntry.type;

        int nLen = cEntry.nLen & 0xFF;
        dirc.records[ptr + 23] = cEntry.nLen;
        System.arraycopy(cEntry.name, 0, dirc.records, ptr + 24, nLen);

        int cLen = cEntry.cLen & 0xFF;
        dirc.records[ptr + 24 + nLen] = cEntry.cLen;
        System.arraycopy(cEntry.comm, 0, dirc.records, ptr + 24 + nLen + 1, cLen);

        l = 25 + nLen + cLen;
        if ((l % 2) == 0) {
            return l;
        } else {
            dirc.records[ptr + l] = (byte) 0;
            return l + 1;
        }

        /* ptr%2 must be == 0, if l%2==0, (ptr+l)%2==0 */
    }

    /*
     * adfEntry2CacheEntry
     *
     * converts one dir entry into a cache entry, and return its future length in records[]
     */

    public static int adfEntry2CacheEntry(BEntryBlock entry, CacheEntry newEntry) {
        int entryLen = 0;

        /* new entry */
        newEntry.header = entry.headerKey;
        if (entry.secType == AdfConstants.ST_FILE) {
            newEntry.size = entry.byteSize;
        } else {
            newEntry.size = 0;
        }
        newEntry.protect = entry.access;
        newEntry.days = (short) entry.days;
        newEntry.mins = (short) entry.mins;
        newEntry.ticks = (short) entry.ticks;
        newEntry.type = (byte) entry.secType;
        newEntry.nLen = entry.nameLen;
        int nLen = newEntry.nLen & 0xFF;
        System.arraycopy(entry.name, 0, newEntry.name, 0, nLen);
        newEntry.name[nLen] = 0;
        newEntry.cLen = entry.commLen;
        int cLen = newEntry.cLen & 0xFF;
        if (cLen > 0) {
            System.arraycopy(entry.comment, 0, newEntry.comm, 0, cLen);
        }

        entryLen = 24 + nLen + 1 + cLen;

        if ((entryLen % 2) == 0) {
            return entryLen;
        } else {
            return entryLen + 1;
        }
    }

    /*
     * adfDelFromCache
     *
     * delete one cache entry from its block. don't do 'records garbage collecting'
     */

    public static AdfError adfDelFromCache(Volume vol, BEntryBlock parent, int headerKey) {
        BDirCacheBlock dirc = new BDirCacheBlock();
        int nSect = 0;
        int prevSect = 0;
        CacheEntry caEntry = new CacheEntry();
        int offset = 0;
        int oldOffset = 0;
        int n = 0;
        boolean found = false;
        int entryLen = 0;
        int i = 0;
        AdfError rc = AdfError.RC_OK;

        prevSect = -1;
        nSect = parent.extension;
        found = false;
        do {
            adfReadDirCBlock(vol, nSect, dirc);
            offset = 0;
            n = 0;
            while (n < dirc.recordsNb && !found) {
                oldOffset = offset;
                int[] p = new int[]{offset};
                adfGetCacheEntry(dirc, p, caEntry);
                offset = p[0];
                found = (caEntry.header == headerKey);
                if (found) {
                    entryLen = offset - oldOffset;
                    if (dirc.recordsNb > 1 || prevSect == -1) {
                        if (n < dirc.recordsNb - 1) {
                            /* not the last of the block : switch the following records */
                            for (i = oldOffset; i < (488 - entryLen); i++) {
                                dirc.records[i] = dirc.records[i + entryLen];
                            }
                            /* and clear the following bytes */
                            for (i = 488 - entryLen; i < 488; i++) {
                                dirc.records[i] = 0;
                            }
                        } else {
                            /* the last record of this cache block */
                            for (i = oldOffset; i < offset; i++) {
                                dirc.records[i] = 0;
                            }
                        }
                        dirc.recordsNb--;
                        if (adfWriteDirCBlock(vol, dirc.headerKey, dirc) != AdfError.RC_OK) {
                            return AdfError.RC_ERROR;
                        }
                    } else {
                        /* dirc.recordsNb ==1 or == 0 , prevSect!=-1 :
                         * the only record in this dirc block and a previous dirc block exists
                         */
                        AdfBitm.adfSetBlockFree(vol, dirc.headerKey);
                        adfReadDirCBlock(vol, prevSect, dirc);
                        dirc.nextDirC = 0;
                        adfWriteDirCBlock(vol, prevSect, dirc);

                        AdfBitm.adfUpdateBitmap(vol);
                    }
                }
                n++;
            }
            prevSect = nSect;
            nSect = dirc.nextDirC;
        } while (nSect != 0 && !found);

        if (!found) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfUpdateCache : entry not found");
            }
        }

        return rc;
    }

    /*
     * adfAddInCache
     *
     */

    public static AdfError adfAddInCache(Volume vol, BEntryBlock parent, BEntryBlock entry) {
        BDirCacheBlock dirc = new BDirCacheBlock();
        BDirCacheBlock newDirc = new BDirCacheBlock();
        int nSect = 0;
        int nCache = 0;
        CacheEntry caEntry = new CacheEntry();
        CacheEntry newEntry = new CacheEntry();
        int offset = 0;
        int n = 0;
        int entryLen = 0;

        entryLen = adfEntry2CacheEntry(entry, newEntry);

        nSect = parent.extension;
        do {
            if (adfReadDirCBlock(vol, nSect, dirc) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            offset = 0;
            n = 0;
            while (n < dirc.recordsNb) {
                int[] p = new int[]{offset};
                adfGetCacheEntry(dirc, p, caEntry);
                offset = p[0];
                n++;
            }

            nSect = dirc.nextDirC;
        } while (nSect != 0);

        /* in the last block */
        if (offset + entryLen <= 488) {
            int[] p = new int[]{offset};
            adfPutCacheEntry(dirc, p, newEntry);
            dirc.recordsNb++;
        } else {
            /* request one new block free */
            nCache = AdfBitm.adfGet1FreeBlock(vol);
            if (nCache == -1) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfCreateDir : nCache==-1");
                }
                return AdfError.RC_VOLFULL;
            }

            /* create a new dircache block */
            // memset(&newDirc,0,512);
            newDirc = new BDirCacheBlock();
            Arrays.fill(newDirc.records, (byte) 0);
            if (parent.secType == AdfConstants.ST_ROOT) {
                newDirc.parent = vol.rootBlock;
            } else if (parent.secType == AdfConstants.ST_DIR) {
                newDirc.parent = parent.headerKey;
            } else {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfAddInCache : unknown secType");
                }
            }
            newDirc.recordsNb = 0;
            newDirc.nextDirC = 0;

            int[] p = new int[]{offset};
            adfPutCacheEntry(dirc, p, newEntry);
            newDirc.recordsNb++;
            if (adfWriteDirCBlock(vol, nCache, newDirc) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            dirc.nextDirC = nCache;
        }
        if (adfWriteDirCBlock(vol, dirc.headerKey, dirc) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfUpdateCache
     *
     */

    public static AdfError adfUpdateCache(Volume vol, BEntryBlock parent, BEntryBlock entry, boolean entryLenChg) {
        BDirCacheBlock dirc = new BDirCacheBlock();
        int nSect = 0;
        CacheEntry caEntry = new CacheEntry();
        CacheEntry newEntry = new CacheEntry();
        int offset = 0;
        int oldOffset = 0;
        int n = 0;
        boolean found = false;
        int i = 0;
        int oLen = 0;
        int nLen = 0;
        int sLen = 0; /* shift length */

        nLen = adfEntry2CacheEntry(entry, newEntry);

        nSect = parent.extension;
        found = false;
        do {
            if (adfReadDirCBlock(vol, nSect, dirc) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            offset = 0;
            n = 0;
            /* search entry to update with its header_key */
            while (n < dirc.recordsNb && !found) {
                oldOffset = offset;
                /* offset is updated */
                int[] p = new int[]{offset};
                adfGetCacheEntry(dirc, p, caEntry);
                offset = p[0];
                oLen = offset - oldOffset;
                sLen = oLen - nLen;
                found = (caEntry.header == newEntry.header);
                if (found) {
                    if (!entryLenChg || oLen == nLen) {
                        /* same length : remplace the old values */
                        int[] pp = new int[]{oldOffset};
                        adfPutCacheEntry(dirc, pp, newEntry);
                        if (adfWriteDirCBlock(vol, dirc.headerKey, dirc) != AdfError.RC_OK) {
                            return AdfError.RC_ERROR;
                        }
                    } else if (oLen > nLen) {
                        /* the new record is shorter, write it,
                         * then shift down the following records
                         */
                        int[] pp = new int[]{oldOffset};
                        adfPutCacheEntry(dirc, pp, newEntry);
                        for (i = oldOffset + nLen; i < (488 - sLen); i++) {
                            dirc.records[i] = dirc.records[i + sLen];
                        }
                        /* then clear the following bytes */
                        for (i = 488 - sLen; i < 488; i++) {
                            dirc.records[i] = (byte) 0;
                        }

                        if (adfWriteDirCBlock(vol, dirc.headerKey, dirc) != AdfError.RC_OK) {
                            return AdfError.RC_ERROR;
                        }
                    } else {
                        /* the new record is larger */
                        adfDelFromCache(vol, parent, entry.headerKey);
                        adfAddInCache(vol, parent, entry);

                    }
                }
                n++;
            }
            nSect = dirc.nextDirC;
        } while (nSect != 0 && !found);

        if (found) {
            if (AdfBitm.adfUpdateBitmap(vol) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfUpdateCache : entry not found");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCreateEmptyCache
     *
     */

    public static AdfError adfCreateEmptyCache(Volume vol, BEntryBlock parent, int nSect) {
        BDirCacheBlock dirc = new BDirCacheBlock();
        int nCache = 0;

        if (nSect == -1) {
            nCache = AdfBitm.adfGet1FreeBlock(vol);
            if (nCache == -1) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfCreateDir : nCache==-1");
                }
                return AdfError.RC_VOLFULL;
            }
        } else {
            nCache = nSect;
        }

        if (parent.extension == 0) {
            parent.extension = nCache;
        }

        // memset(&dirc,0, sizeof(struct bDirCacheBlock));
        dirc = new BDirCacheBlock();
        Arrays.fill(dirc.records, (byte) 0);

        if (parent.secType == AdfConstants.ST_ROOT) {
            dirc.parent = vol.rootBlock;
        } else if (parent.secType == AdfConstants.ST_DIR) {
            dirc.parent = parent.headerKey;
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCreateEmptyCache : unknown secType");
            }
        }

        dirc.recordsNb = 0;
        dirc.nextDirC = 0;

        if (adfWriteDirCBlock(vol, nCache, dirc) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /** Overload for root parent (BRootBlock). */

    public static AdfError adfCreateEmptyCache(Volume vol, BRootBlock parent, int nSect) {
        BDirCacheBlock dirc = new BDirCacheBlock();
        int nCache = 0;

        if (nSect == -1) {
            nCache = AdfBitm.adfGet1FreeBlock(vol);
            if (nCache == -1) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfCreateDir : nCache==-1");
                }
                return AdfError.RC_VOLFULL;
            }
        } else {
            nCache = nSect;
        }

        if (parent.extension == 0) {
            parent.extension = nCache;
        }

        dirc = new BDirCacheBlock();
        Arrays.fill(dirc.records, (byte) 0);

        // parent is root
        dirc.parent = vol.rootBlock;

        dirc.recordsNb = 0;
        dirc.nextDirC = 0;

        if (adfWriteDirCBlock(vol, nCache, dirc) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfReadDirCBlock
     *
     */

    public static AdfError adfReadDirCBlock(Volume vol, int nSect, BDirCacheBlock dirc) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BDirCacheBlock tmp = BDirCacheBlock.read(bb, 0);
        // copy into dirc
        dirc.type = tmp.type;
        dirc.headerKey = tmp.headerKey;
        dirc.parent = tmp.parent;
        dirc.recordsNb = tmp.recordsNb;
        dirc.nextDirC = tmp.nextDirC;
        dirc.checkSum = tmp.checkSum;
        System.arraycopy(tmp.records, 0, dirc.records, 0, tmp.records.length);

        if (dirc.checkSum != AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadDirCBlock : invalid checksum");
            }
        }
        if (dirc.type != AdfConstants.T_DIRC) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadDirCBlock : T_DIRC not found");
            }
        }
        if (dirc.headerKey != nSect) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadDirCBlock : headerKey!=nSect");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteDirCblock
     *
     */

    public static AdfError adfWriteDirCBlock(Volume vol, int nSect, BDirCacheBlock dirc) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;

        dirc.type = AdfConstants.T_DIRC;
        dirc.headerKey = nSect;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        dirc.write(bb, 0);

        newSum = AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE);
        swLong(buf, 20, newSum);

        if (AdfDisk.adfWriteBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    // ------------------------------------------------------------------
    // Helpers — keep original C names where possible
    // ------------------------------------------------------------------

    private static void swLong(byte[] buf, int off, long val) {
        buf[off] = (byte) ((val >> 24) & 0xFF);
        buf[off + 1] = (byte) ((val >> 16) & 0xFF);
        buf[off + 2] = (byte) ((val >> 8) & 0xFF);
        buf[off + 3] = (byte) (val & 0xFF);
    }

    private static void swShort(byte[] buf, int off, int val) {
        buf[off] = (byte) ((val >> 8) & 0xFF);
        buf[off + 1] = (byte) (val & 0xFF);
    }

    private static AdfList newCell(AdfList list, Object content) {
        AdfList cell = new AdfList();
        if (cell == null) {
            if (adfEnv != null && adfEnv.eFct != null) {
                adfEnv.eFct.call("newCell : malloc");
            }
            return null;
        }
        cell.content = content;
        cell.next = null;
        cell.subdir = null;
        if (list != null) {
            list.next = cell;
        }
        return cell;
    }

    private static void adfFreeDirList(AdfList list) {
        AdfList cell = list;
        while (cell != null) {
            AdfList next = cell.next;
            if (cell.subdir != null) {
                adfFreeDirList(cell.subdir);
            }
            // content (Entry) will be GC'd
            cell = next;
        }
    }

    private static void adfFreeEntry(Entry entry) {
        // no-op in Java (GC)
    }

    /*
     * adfReadEntryBlock
     *
     */

    private static AdfError adfReadEntryBlock(Volume vol, int nSect, BEntryBlock ent) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BEntryBlock tmp = BEntryBlock.read(bb, 0);
        // copy into ent
        ent.type = tmp.type;
        ent.headerKey = tmp.headerKey;
        System.arraycopy(tmp.r1, 0, ent.r1, 0, tmp.r1.length);
        ent.checkSum = tmp.checkSum;
        System.arraycopy(tmp.hashTable, 0, ent.hashTable, 0, tmp.hashTable.length);
        System.arraycopy(tmp.r2, 0, ent.r2, 0, tmp.r2.length);
        ent.access = tmp.access;
        ent.byteSize = tmp.byteSize;
        ent.commLen = tmp.commLen;
        System.arraycopy(tmp.comment, 0, ent.comment, 0, tmp.comment.length);
        System.arraycopy(tmp.r3, 0, ent.r3, 0, tmp.r3.length);
        ent.days = tmp.days;
        ent.mins = tmp.mins;
        ent.ticks = tmp.ticks;
        ent.nameLen = tmp.nameLen;
        System.arraycopy(tmp.name, 0, ent.name, 0, tmp.name.length);
        ent.r4 = tmp.r4;
        ent.realEntry = tmp.realEntry;
        ent.nextLink = tmp.nextLink;
        System.arraycopy(tmp.r5, 0, ent.r5, 0, tmp.r5.length);
        ent.nextSameHash = tmp.nextSameHash;
        ent.parent = tmp.parent;
        ent.extension = tmp.extension;
        ent.secType = tmp.secType;

        if (ent.checkSum != AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadEntryBlock : invalid checksum");
            }
            return AdfError.RC_ERROR;
        }
        if (ent.type != AdfConstants.T_HEADER) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadEntryBlock : T_HEADER id not found");
            }
            return AdfError.RC_ERROR;
        }
        return AdfError.RC_OK;
    }

    private static void adfDays2Date(int days, int[] yy, int[] mm, int[] dd) {
        int y = 1978;
        int m = 1;
        int nd = 0;
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        if (adfIsLeap(y)) {
            nd = 366;
        } else {
            nd = 365;
        }
        while (days >= nd) {
            days -= nd;
            y++;
            if (adfIsLeap(y)) {
                nd = 366;
            } else {
                nd = 365;
            }
        }

        m = 1;
        if (adfIsLeap(y)) {
            jm[1] = 29;
        }
        while (days >= jm[m - 1]) {
            days -= jm[m - 1];
            m++;
        }

        yy[0] = y;
        mm[0] = m;
        dd[0] = days + 1;
    }

    private static boolean adfIsLeap(int y) {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
    }
}
