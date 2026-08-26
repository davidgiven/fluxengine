/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_salv.c / adf_salv.h
 *
 *  $Id$
 *
 *  undelete and salvage code : EXPERIMENTAL !
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
 * Java port of {@code adf_salv.c} / {@code adf_salv.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data.
 * High-level objects use normal Java classes ({@link GenBlock}, {@link AdfList},
 * {@link FileBlocks}). Return codes use {@link AdfError}.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfSalv {

    private AdfSalv() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    /*
     * adfFreeGenBlock
     *
     */

    public static void adfFreeGenBlock(GenBlock block) {
        if (block.name != null) {
            block.name = null;
        }
    }

    /*
     * adfFreeDelList
     *
     */

    public static void adfFreeDelList(AdfList list) {
        AdfList cell = list;

        cell = list;
        while (cell != null) {
            adfFreeGenBlock((GenBlock) cell.content);
            cell = cell.next;
        }
        AdfUtil.freeList(list);
    }

    /*
     * adfGetDelEnt
     *
     */

    public static AdfList adfGetDelEnt(Volume vol) {
        GenBlock block = null;
        int i = 0;
        AdfList list = null;
        AdfList head = null;
        boolean delEnt = false;

        list = head = null;
        block = null;
        delEnt = true;
        for (i = vol.firstBlock; i <= vol.lastBlock; i++) {
            if (AdfBitm.adfIsBlockFree(vol, i)) {
                if (delEnt) {
                    block = new GenBlock();
                    if (block == null) {
                        return null;
                    }
                    /*printf("%p\n",block);*/
                }

                adfReadGenBlock(vol, i, block);

                delEnt = (block.type == AdfConstants.T_HEADER
                    && (block.secType == AdfConstants.ST_DIR || block.secType == AdfConstants.ST_FILE));

                if (delEnt) {
                    if (head == null) {
                        list = head = AdfUtil.newCell(null, (Object) block);
                    } else {
                        list = AdfUtil.newCell(list, (Object) block);
                    }
                }
            }
        }

        if (block != null && list != null && block != list.content) {
            /* free(block); */
            /*        printf("%p\n",block);*/
        }
        return head;
    }

    /*
     * adfReadGenBlock
     *
     */

    public static AdfError adfReadGenBlock(Volume vol, int nSect, GenBlock block) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        int len = 0;
        String nameStr = "";

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        block.type = AdfEndian.Long(buf, 0);
        block.secType = AdfEndian.Long(buf, vol.blockSize - 4);
        block.sect = nSect;
        block.name = null;

        if (block.type == AdfConstants.T_HEADER) {
            switch (block.secType) {
            case AdfConstants.ST_FILE:
            case AdfConstants.ST_DIR:
            case AdfConstants.ST_LFILE:
            case AdfConstants.ST_LDIR:
                len = AdfConstants.min(AdfConstants.MAXNAMELEN, buf[vol.blockSize - 80] & 0xFF);
                byte[] nameBytes = new byte[len];
                System.arraycopy(buf, vol.blockSize - 79, nameBytes, 0, len);
                nameStr = new String(nameBytes);
                block.name = nameStr;
                block.parent = AdfEndian.Long(buf, vol.blockSize - 12);
                break;
            case AdfConstants.ST_ROOT:
                break;
            default:
                break;
            }
        }
        return AdfError.RC_OK;
    }

    /*
     * adfCheckParent
     *
     */

    public static AdfError adfCheckParent(Volume vol, int pSect) {
        GenBlock block = new GenBlock();

        if (AdfBitm.adfIsBlockFree(vol, pSect)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCheckParent : parent doesn't exists");
            }
            return AdfError.RC_ERROR;
        }

        /* verify if parent is a DIR or ROOT */
        adfReadGenBlock(vol, pSect, block);
        if (block.type != AdfConstants.T_HEADER
            || (block.secType != AdfConstants.ST_DIR && block.secType != AdfConstants.ST_ROOT)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCheckParent : parent secType is incorrect");
            }
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfUndelDir
     *
     */

    public static AdfError adfUndelDir(Volume vol, int pSect, int nSect, BDirBlock entry) {
        AdfError rc = AdfError.RC_OK;
        BEntryBlock parent = new BEntryBlock();
        String nameStr = "";

        /* check if the given parent sector pointer seems OK */
        if ((rc = adfCheckParent(vol, pSect)) != AdfError.RC_OK) {
            return rc;
        }

        if (pSect != entry.parent) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfUndelDir : the given parent sector isn't the entry parent");
            }
            return AdfError.RC_ERROR;
        }

        if (!AdfBitm.adfIsBlockFree(vol, entry.headerKey)) {
            return AdfError.RC_ERROR;
        }
        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF) && !AdfBitm.adfIsBlockFree(vol, entry.extension)) {
            return AdfError.RC_ERROR;
        }

        if (AdfDir.adfReadEntryBlock(vol, pSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        int nameLen = entry.nameLen & 0xFF;
        byte[] nameBytes = new byte[nameLen];
        System.arraycopy(entry.dirName, 0, nameBytes, 0, nameLen);
        nameStr = new String(nameBytes);
        /* insert the entry in the parent hashTable, with the headerKey sector pointer */
        AdfBitm.adfSetBlockUsed(vol, entry.headerKey);
        AdfDir.adfCreateEntry(vol, parent, nameStr, entry.headerKey);

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            AdfCache.adfAddInCache(vol, parent, dirBlockToEntryBlock(entry));
            AdfBitm.adfSetBlockUsed(vol, entry.extension);
        }

        AdfBitm.adfUpdateBitmap(vol);

        return AdfError.RC_OK;
    }

    /*
     * adfUndelFile
     *
     */

    public static AdfError adfUndelFile(Volume vol, int pSect, int nSect, BFileHeaderBlock entry) {
        int i = 0;
        String nameStr = "";
        BEntryBlock parent = new BEntryBlock();
        AdfError rc = AdfError.RC_OK;
        FileBlocks fileBlocks = new FileBlocks();

        /* check if the given parent sector pointer seems OK */
        if ((rc = adfCheckParent(vol, pSect)) != AdfError.RC_OK) {
            return rc;
        }

        if (pSect != entry.parent) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfUndelFile : the given parent sector isn't the entry parent");
            }
            return AdfError.RC_ERROR;
        }

        AdfFile.adfGetFileBlocks(vol, entry, fileBlocks);

        for (i = 0; i < fileBlocks.nbData; i++) {
            if (!AdfBitm.adfIsBlockFree(vol, fileBlocks.data[i])) {
                return AdfError.RC_ERROR;
            } else {
                AdfBitm.adfSetBlockUsed(vol, fileBlocks.data[i]);
            }
        }
        for (i = 0; i < fileBlocks.nbExtens; i++) {
            if (!AdfBitm.adfIsBlockFree(vol, fileBlocks.extens[i])) {
                return AdfError.RC_ERROR;
            } else {
                AdfBitm.adfSetBlockUsed(vol, fileBlocks.extens[i]);
            }
        }

        /* free(fileBlocks.data); free(fileBlocks.extens); handled by GC */

        if (AdfDir.adfReadEntryBlock(vol, pSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        int nameLen = entry.nameLen & 0xFF;
        byte[] nameBytes = new byte[nameLen];
        System.arraycopy(entry.fileName, 0, nameBytes, 0, nameLen);
        nameStr = new String(nameBytes);
        /* insert the entry in the parent hashTable, with the headerKey sector pointer */
        AdfDir.adfCreateEntry(vol, parent, nameStr, entry.headerKey);

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            AdfCache.adfAddInCache(vol, parent, fileHeaderToEntryBlock(entry));
        }

        AdfBitm.adfUpdateBitmap(vol);

        return AdfError.RC_OK;
    }

    /*
     * adfUndelEntry
     *
     */

    public static AdfError adfUndelEntry(Volume vol, int parent, int nSect) {
        BEntryBlock entry = new BEntryBlock();

        AdfDir.adfReadEntryBlock(vol, nSect, entry);

        switch (entry.secType) {
        case AdfConstants.ST_FILE:
            adfUndelFile(vol, parent, nSect, entryBlockToFileHeader(entry));
            break;
        case AdfConstants.ST_DIR:
            adfUndelDir(vol, parent, nSect, entryBlockToDirBlock(entry));
            break;
        default:
            break;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCheckFile
     *
     */

    public static AdfError adfCheckFile(Volume vol, int nSect, BFileHeaderBlock file, int level) {
        BFileExtBlock extBlock = new BFileExtBlock();
        BOFSDataBlock dataBlock = new BOFSDataBlock();
        FileBlocks fileBlocks = new FileBlocks();
        int n = 0;

        AdfFile.adfGetFileBlocks(vol, file, fileBlocks);
        /*printf("data %ld ext %ld\n",fileBlocks.nbData,fileBlocks.nbExtens);*/
        if (AdfConstants.isOFS(vol.dosType & 0xFF)) {
            /* checks OFS datablocks */
            for (n = 0; n < fileBlocks.nbData; n++) {
                /*printf("%ld\n",fileBlocks.data[n]);*/
                ByteBuffer curData = ByteBuffer.allocate(512).order(ByteOrder.BIG_ENDIAN);
                AdfFile.adfReadDataBlock(vol, fileBlocks.data[n], curData);
                /* need to interpret as BOFSDataBlock */
                BOFSDataBlock d = BOFSDataBlock.read(curData.duplicate().order(ByteOrder.BIG_ENDIAN), 0);
                if (d.headerKey != fileBlocks.header) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCheckFile : headerKey incorrect");
                    }
                }
                if (d.seqNum != n + 1) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCheckFile : seqNum incorrect");
                    }
                }
                if (n < fileBlocks.nbData - 1) {
                    if (d.nextData != fileBlocks.data[n + 1]) {
                        if (adfEnv != null && adfEnv.wFct != null) {
                            adfEnv.wFct.call("adfCheckFile : nextData incorrect");
                        }
                    }
                    if (d.dataSize != vol.datablockSize) {
                        if (adfEnv != null && adfEnv.wFct != null) {
                            adfEnv.wFct.call("adfCheckFile : dataSize incorrect");
                        }
                    }
                } else { /* last datablock */
                    if (d.nextData != 0) {
                        if (adfEnv != null && adfEnv.wFct != null) {
                            adfEnv.wFct.call("adfCheckFile : nextData incorrect");
                        }
                    }
                }
                dataBlock = d;
            }
        }
        for (n = 0; n < fileBlocks.nbExtens; n++) {
            AdfFile.adfReadFileExtBlock(vol, fileBlocks.extens[n], extBlock);
            if (extBlock.parent != file.headerKey) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfCheckFile : extBlock parent incorrect");
                }
            }
            if (n < fileBlocks.nbExtens - 1) {
                if (extBlock.extension != fileBlocks.extens[n + 1]) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCheckFile : nextData incorrect");
                    }
                }
            } else {
                if (extBlock.extension != 0) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCheckFile : nextData incorrect");
                    }
                }
            }
        }

        /* free(fileBlocks.data); free(fileBlocks.extens); */

        return AdfError.RC_OK;
    }

    /*
     * adfCheckDir
     *
     */

    public static AdfError adfCheckDir(Volume vol, int nSect, BDirBlock dir, int level) {

        return AdfError.RC_OK;
    }

    /*
     * adfCheckEntry
     *
     */

    public static AdfError adfCheckEntry(Volume vol, int nSect, int level) {
        BEntryBlock entry = new BEntryBlock();
        AdfError rc = AdfError.RC_OK;

        AdfDir.adfReadEntryBlock(vol, nSect, entry);

        switch (entry.secType) {
        case AdfConstants.ST_FILE:
            rc = adfCheckFile(vol, nSect, entryBlockToFileHeader(entry), level);
            break;
        case AdfConstants.ST_DIR:
            rc = adfCheckDir(vol, nSect, entryBlockToDirBlock(entry), level);
            break;
        default:
            /*        printf("adfCheckEntry : not supported\n");*/                    /* BV */
            rc = AdfError.RC_ERROR;
            break;
        }

        return rc;
    }

    // ------------------------------------------------------------------
    // Helpers — keep original C names where possible
    // ------------------------------------------------------------------

    private static BDirBlock entryBlockToDirBlock(BEntryBlock e) {
        BDirBlock d = new BDirBlock();
        d.type = e.type;
        d.headerKey = e.headerKey;
        d.highSeq = 0;
        d.hashTableSize = 0;
        d.r1 = e.r1[0];
        d.checkSum = e.checkSum;
        System.arraycopy(e.hashTable, 0, d.hashTable, 0, e.hashTable.length);
        d.r2[0] = e.r2[0];
        d.r2[1] = e.r2[1];
        d.access = e.access;
        d.r4 = 0;
        d.commLen = e.commLen;
        System.arraycopy(e.comment, 0, d.comment, 0, Math.min(e.comment.length, d.comment.length));
        d.days = e.days;
        d.mins = e.mins;
        d.ticks = e.ticks;
        d.nameLen = e.nameLen;
        System.arraycopy(e.name, 0, d.dirName, 0, Math.min(e.name.length, d.dirName.length));
        d.r6 = e.r4;
        d.real = e.realEntry;
        d.nextLink = e.nextLink;
        System.arraycopy(e.r5, 0, d.r7, 0, Math.min(e.r5.length, d.r7.length));
        d.nextSameHash = e.nextSameHash;
        d.parent = e.parent;
        d.extension = e.extension;
        d.secType = e.secType;
        return d;
    }

    private static BEntryBlock dirBlockToEntryBlock(BDirBlock d) {
        BEntryBlock e = new BEntryBlock();
        e.type = d.type;
        e.headerKey = d.headerKey;
        e.r1[0] = d.r1;
        e.r1[1] = 0;
        e.r1[2] = 0;
        e.checkSum = d.checkSum;
        System.arraycopy(d.hashTable, 0, e.hashTable, 0, d.hashTable.length);
        e.r2[0] = d.r2[0];
        e.r2[1] = d.r2[1];
        e.access = d.access;
        e.byteSize = 0;
        e.commLen = d.commLen;
        System.arraycopy(d.comment, 0, e.comment, 0, Math.min(d.comment.length, e.comment.length));
        e.days = d.days;
        e.mins = d.mins;
        e.ticks = d.ticks;
        e.nameLen = d.nameLen;
        System.arraycopy(d.dirName, 0, e.name, 0, Math.min(d.dirName.length, e.name.length));
        e.r4 = d.r6;
        e.realEntry = d.real;
        e.nextLink = d.nextLink;
        System.arraycopy(d.r7, 0, e.r5, 0, Math.min(d.r7.length, e.r5.length));
        e.nextSameHash = d.nextSameHash;
        e.parent = d.parent;
        e.extension = d.extension;
        e.secType = d.secType;
        return e;
    }

    private static BFileHeaderBlock entryBlockToFileHeader(BEntryBlock e) {
        BFileHeaderBlock f = new BFileHeaderBlock();
        f.type = e.type;
        f.headerKey = e.headerKey;
        f.highSeq = e.r1[0];
        f.dataSize = e.r1[1];
        f.firstData = e.r1[2];
        f.checkSum = e.checkSum;
        System.arraycopy(e.hashTable, 0, f.dataBlocks, 0, AdfConstants.MAX_DATABLK);
        f.r1 = e.r2[0];
        f.r2 = e.r2[1];
        f.access = e.access;
        f.byteSize = e.byteSize & 0xFFFFFFFFL;
        f.commLen = e.commLen;
        System.arraycopy(e.comment, 0, f.comment, 0, Math.min(e.comment.length, f.comment.length));
        f.days = e.days;
        f.mins = e.mins;
        f.ticks = e.ticks;
        f.nameLen = e.nameLen;
        System.arraycopy(e.name, 0, f.fileName, 0, Math.min(e.name.length, f.fileName.length));
        f.r4 = e.r4;
        f.real = e.realEntry;
        f.nextLink = e.nextLink;
        System.arraycopy(e.r5, 0, f.r5, 0, Math.min(e.r5.length, f.r5.length));
        f.nextSameHash = e.nextSameHash;
        f.parent = e.parent;
        f.extension = e.extension;
        f.secType = e.secType;
        return f;
    }

    private static BEntryBlock fileHeaderToEntryBlock(BFileHeaderBlock f) {
        BEntryBlock e = new BEntryBlock();
        e.type = f.type;
        e.headerKey = f.headerKey;
        e.r1[0] = f.highSeq;
        e.r1[1] = f.dataSize;
        e.r1[2] = f.firstData;
        e.checkSum = f.checkSum;
        System.arraycopy(f.dataBlocks, 0, e.hashTable, 0, AdfConstants.MAX_DATABLK);
        e.r2[0] = f.r1;
        e.r2[1] = f.r2;
        e.access = f.access;
        e.byteSize = (int) (f.byteSize & 0xFFFFFFFFL);
        e.commLen = f.commLen;
        System.arraycopy(f.comment, 0, e.comment, 0, Math.min(f.comment.length, e.comment.length));
        e.days = f.days;
        e.mins = f.mins;
        e.ticks = f.ticks;
        e.nameLen = f.nameLen;
        System.arraycopy(f.fileName, 0, e.name, 0, Math.min(f.fileName.length, e.name.length));
        e.r4 = f.r4;
        e.realEntry = f.real;
        e.nextLink = f.nextLink;
        System.arraycopy(f.r5, 0, e.r5, 0, Math.min(f.r5.length, e.r5.length));
        e.nextSameHash = f.nextSameHash;
        e.parent = f.parent;
        e.extension = f.extension;
        e.secType = f.secType;
        return e;
    }
}
