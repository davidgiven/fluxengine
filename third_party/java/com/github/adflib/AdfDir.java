/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_dir.c / adf_dir.h
 *
 *  $Id$
 *
 *  directory code
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
 * Java port of {@code adf_dir.c} / {@code adf_dir.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data.
 * High-level objects use normal Java classes ({@link Entry}, {@link AdfList}).
 * Return codes use {@link AdfError} with out-parameters as single-element arrays.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfDir {

    private AdfDir() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    public static Entry adfFindEntry(Volume vol, String name) {
        int nSect = 0;
        BEntryBlock entryBlk = new BEntryBlock();
        BEntryBlock parent = new BEntryBlock();
        Entry entry = null;

        adfReadEntryBlock(vol, vol.curDirPtr, parent);

        nSect = adfNameToEntryBlk(vol, parent.hashTable, name, entryBlk, null);
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfRenameEntry : existing entry not found");
            }
            return null;
        }

        entry = new Entry();
        if (adfEntBlock2Entry(entryBlk, entry) != AdfError.RC_OK) {
            return null;
        }
        entry.sector = nSect;

        return entry;
    }

    /*
     * adfRenameEntry
     *
     */

    public static AdfError adfRenameEntry(Volume vol, int pSect, String oldName,
            int nPSect, String newName) {
        BEntryBlock parent = new BEntryBlock();
        BEntryBlock previous = new BEntryBlock();
        BEntryBlock entry = new BEntryBlock();
        BEntryBlock nParent = new BEntryBlock();
        int nSect2 = 0;
        int nSect = 0;
        int prevSect = 0;
        int tmpSect = 0;
        int hashValueO = 0;
        int hashValueN = 0;
        int len = 0;
        byte[] name2 = new byte[AdfConstants.MAXNAMELEN + 1];
        byte[] name3 = new byte[AdfConstants.MAXNAMELEN + 1];
        boolean intl = false;
        AdfError rc = AdfError.RC_OK;
        int[] prevSectArr = new int[1];

        if ((pSect == nPSect) && (oldName.equals(newName))) {
            return AdfError.RC_OK;
        }

        intl = AdfConstants.isINTL(vol.dosType & 0xFF) || AdfConstants.isDIRCACHE(vol.dosType & 0xFF);
        len = AdfConstants.min(AdfConstants.MAXNAMELEN, newName.length());
        myToUpper(name2, newName.getBytes(), len, intl);
        myToUpper(name3, oldName.getBytes(), oldName.length(), intl);
        /* newName == oldName ? */

        if (adfReadEntryBlock(vol, pSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        hashValueO = adfGetHashValue(oldName.getBytes(), intl);

        nSect = adfNameToEntryBlk(vol, parent.hashTable, oldName, entry, prevSectArr);
        prevSect = prevSectArr[0];
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfRenameEntry : existing entry not found");
            }
            return AdfError.RC_ERROR;
        }

        /* change name and parent dir */
        entry.nameLen = (byte) AdfConstants.min(31, newName.length());
        byte[] newNameBytes = newName.getBytes();
        for (int i = 0; i < (entry.nameLen & 0xFF); i++) {
            entry.name[i] = newNameBytes[i];
        }
        entry.parent = nPSect;
        tmpSect = entry.nextSameHash;

        entry.nextSameHash = 0;
        if (adfWriteEntryBlock(vol, nSect, entry) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        /* del from the oldname list */

        /* in hashTable */
        if (prevSect == 0) {
            parent.hashTable[hashValueO] = tmpSect;
            if (parent.secType == AdfConstants.ST_ROOT) {
                rc = adfWriteRootBlockFromEntry(vol, pSect, parent);
            } else {
                rc = adfWriteDirBlockFromEntry(vol, pSect, parent);
            }
            if (rc != AdfError.RC_OK) {
                return rc;
            }
        } else {
            /* in linked list */
            if (adfReadEntryBlock(vol, prevSect, previous) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            /* entry.nextSameHash (tmpSect) could be == 0 */
            previous.nextSameHash = tmpSect;
            if (adfWriteEntryBlock(vol, prevSect, previous) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
        }

        if (adfReadEntryBlock(vol, nPSect, nParent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        hashValueN = adfGetHashValue(newName.getBytes(), intl);
        nSect2 = nParent.hashTable[hashValueN];
        /* no list */
        if (nSect2 == 0) {
            nParent.hashTable[hashValueN] = nSect;
            if (nParent.secType == AdfConstants.ST_ROOT) {
                rc = adfWriteRootBlockFromEntry(vol, nPSect, nParent);
            } else {
                rc = adfWriteDirBlockFromEntry(vol, nPSect, nParent);
            }
        } else {
            /* a list exists : addition at the end */
            /* len = strlen(newName);
                       * name2 == newName
                       */
            do {
                if (adfReadEntryBlock(vol, nSect2, previous) != AdfError.RC_OK) {
                    return AdfError.RC_ERROR;
                }
                if ((previous.nameLen & 0xFF) == len) {
                    myToUpper(name3, previous.name, previous.nameLen & 0xFF, intl);
                    if (strncmp(name3, name2, len) == 0) {
                        if (adfEnv != null && adfEnv.wFct != null) {
                            adfEnv.wFct.call("adfRenameEntry : entry already exists");
                        }
                        return AdfError.RC_ERROR;
                    }
                }
                nSect2 = previous.nextSameHash;
            } while (nSect2 != 0);

            previous.nextSameHash = nSect;
            if (previous.secType == AdfConstants.ST_DIR) {
                rc = adfWriteDirBlockFromEntry(vol, previous.headerKey, previous);
            } else if (previous.secType == AdfConstants.ST_FILE) {
                rc = AdfFile.adfWriteFileHdrBlock(vol, previous.headerKey,
                        entryBlockToFileHeader(previous));
            } else {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfRenameEntry : unknown entry type");
                }
                rc = AdfError.RC_ERROR;
            }

        }
        if (rc != AdfError.RC_OK) {
            return rc;
        }

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            if (pSect == nPSect) {
                AdfCache.adfUpdateCache(vol, parent, entry, true);
            } else {
                AdfCache.adfDelFromCache(vol, parent, entry.headerKey);
                AdfCache.adfAddInCache(vol, nParent, entry);
            }
        }
        /*
        if (isDIRCACHE(vol->dosType) && pSect!=nPSect) {
            adfUpdateCache(vol, &nParent, (struct bEntryBlock*)&entry,TRUE);
        }
        */
        return AdfError.RC_OK;
    }

    /*
     * adfRemoveEntry
     *
     */

    public static AdfError adfRemoveEntry(Volume vol, int pSect, String name) {
        BEntryBlock parent = new BEntryBlock();
        BEntryBlock previous = new BEntryBlock();
        BEntryBlock entry = new BEntryBlock();
        int nSect2 = 0;
        int nSect = 0;
        int hashVal = 0;
        boolean intl = false;
        int[] nSect2Arr = new int[1];

        if (adfReadEntryBlock(vol, pSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        nSect = adfNameToEntryBlk(vol, parent.hashTable, name, entry, nSect2Arr);
        nSect2 = nSect2Arr[0];
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfRemoveEntry : entry '" + name + "' not found");
            }
            return AdfError.RC_ERROR;
        }
        /* if it is a directory, is it empty ? */
        if (entry.secType == AdfConstants.ST_DIR && !isDirEmpty(entryBlockToDirBlock(entry))) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfRemoveEntry : directory '" + name + "' not empty");
            }
            return AdfError.RC_ERROR;
        }
        /*    printf("name=%s  nSect2=%ld\n",name, nSect2);*/

        /* in parent hashTable */
        if (nSect2 == 0) {
            intl = AdfConstants.isINTL(vol.dosType & 0xFF) || AdfConstants.isDIRCACHE(vol.dosType & 0xFF);
            hashVal = adfGetHashValue(name.getBytes(), intl);
            /*printf("hashTable=%d nexthash=%d\n",parent.hashTable[hashVal],
             entry.nextSameHash);*/
            parent.hashTable[hashVal] = entry.nextSameHash;
            if (adfWriteEntryBlock(vol, pSect, parent) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
        }
        /* in linked list */ else {
            if (adfReadEntryBlock(vol, nSect2, previous) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            previous.nextSameHash = entry.nextSameHash;
            if (adfWriteEntryBlock(vol, nSect2, previous) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
        }

        if (entry.secType == AdfConstants.ST_FILE) {
            AdfFile.adfFreeFileBlocks(vol, entryBlockToFileHeader(entry));
            AdfBitm.adfSetBlockFree(vol, nSect);
            if (adfEnv != null && adfEnv.useNotify && adfEnv.notifyFct != null) {
                adfEnv.notifyFct.notify(pSect, AdfConstants.ST_FILE);
            }
        } else if (entry.secType == AdfConstants.ST_DIR) {
            AdfBitm.adfSetBlockFree(vol, nSect);
            /* free dir cache block : the directory must be empty, so there's only one cache block */
            if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
                AdfBitm.adfSetBlockFree(vol, entry.extension);
            }
            if (adfEnv != null && adfEnv.useNotify && adfEnv.notifyFct != null) {
                adfEnv.notifyFct.notify(pSect, AdfConstants.ST_DIR);
            }
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfRemoveEntry : secType " + entry.secType + " not supported");
            }
            return AdfError.RC_ERROR;
        }

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            AdfCache.adfDelFromCache(vol, parent, entry.headerKey);
        }

        AdfBitm.adfUpdateBitmap(vol);

        return AdfError.RC_OK;
    }

    /*
     * adfSetEntryComment
     *
     */

    public static AdfError adfSetEntryComment(Volume vol, int parSect, String name,
            String newCmt) {
        BEntryBlock parent = new BEntryBlock();
        BEntryBlock entry = new BEntryBlock();
        int nSect = 0;

        if (adfReadEntryBlock(vol, parSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        nSect = adfNameToEntryBlk(vol, parent.hashTable, name, entry, null);
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfSetEntryComment : entry not found");
            }
            return AdfError.RC_ERROR;
        }

        entry.commLen = (byte) AdfConstants.min(AdfConstants.MAXCMMTLEN, newCmt.length());
        byte[] cmtBytes = newCmt.getBytes();
        for (int i = 0; i < (entry.commLen & 0xFF); i++) {
            entry.comment[i] = cmtBytes[i];
        }

        if (entry.secType == AdfConstants.ST_DIR) {
            adfWriteDirBlock(vol, nSect, entryBlockToDirBlock(entry));
        } else if (entry.secType == AdfConstants.ST_FILE) {
            AdfFile.adfWriteFileHdrBlock(vol, nSect, entryBlockToFileHeader(entry));
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfSetEntryComment : entry secType incorrect");
            }
        }

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            AdfCache.adfUpdateCache(vol, parent, entry, true);
        }

        return AdfError.RC_OK;
    }

    /*
     * adfSetEntryAccess
     *
     */

    public static AdfError adfSetEntryAccess(Volume vol, int parSect, String name,
            int newAcc) {
        BEntryBlock parent = new BEntryBlock();
        BEntryBlock entry = new BEntryBlock();
        int nSect = 0;

        if (adfReadEntryBlock(vol, parSect, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        nSect = adfNameToEntryBlk(vol, parent.hashTable, name, entry, null);
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfSetEntryAccess : entry not found");
            }
            return AdfError.RC_ERROR;
        }

        entry.access = newAcc;
        if (entry.secType == AdfConstants.ST_DIR) {
            adfWriteDirBlock(vol, nSect, entryBlockToDirBlock(entry));
        } else if (entry.secType == AdfConstants.ST_FILE) {
            AdfFile.adfWriteFileHdrBlock(vol, nSect, entryBlockToFileHeader(entry));
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfSetEntryAccess : entry secType incorrect");
            }
        }

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            AdfCache.adfUpdateCache(vol, parent, entry, false);
        }

        return AdfError.RC_OK;
    }

    /*
     * isDirEmpty
     *
     */

    public static boolean isDirEmpty(BDirBlock dir) {
        int i = 0;

        for (i = 0; i < AdfConstants.HT_SIZE; i++) {
            if (dir.hashTable[i] != 0) {
                return false;
            }
        }

        return true;
    }

    /*
     * adfFreeDirList
     *
     */

    public static void adfFreeDirList(AdfList list) {
        AdfList cell = list;

        while (cell != null) {
            AdfList next = cell.next;
            /* adfFreeEntry(cell->content); */
            if (cell.subdir != null) {
                adfFreeDirList(cell.subdir);
            }
            cell = next;
        }
        /* freeList(root); */
    }

    /*
     * adfGetRDirEnt
     *
     */

    public static AdfList adfGetRDirEnt(Volume vol, int nSect, boolean recurs) {
        BEntryBlock entryBlk = new BEntryBlock();
        AdfList cell = null;
        AdfList head = null;
        int i = 0;
        Entry entry = null;
        int nextSector = 0;
        int[] hashTable = null;
        BEntryBlock parent = new BEntryBlock();

        if (adfEnv != null && adfEnv.useDirCache && AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            return AdfCache.adfGetDirEntCache(vol, nSect, recurs);
        }

        if (adfReadEntryBlock(vol, nSect, parent) != AdfError.RC_OK) {
            return null;
        }

        hashTable = parent.hashTable;
        cell = null;
        head = null;
        for (i = 0; i < AdfConstants.HT_SIZE; i++) {
            if (hashTable[i] != 0) {
                entry = new Entry();
                if (entry == null) {
                    adfFreeDirList(head);
                    if (adfEnv != null && adfEnv.eFct != null) {
                        adfEnv.eFct.call("adfGetDirEnt : malloc");
                    }
                    return null;
                }
                if (adfReadEntryBlock(vol, hashTable[i], entryBlk) != AdfError.RC_OK) {
                    adfFreeDirList(head);
                    return null;
                }
                if (adfEntBlock2Entry(entryBlk, entry) != AdfError.RC_OK) {
                    adfFreeDirList(head);
                    return null;
                }
                entry.sector = hashTable[i];

                if (head == null) {
                    head = newCell(null, entry);
                    cell = head;
                } else {
                    cell = newCell(cell, entry);
                }
                if (cell == null) {
                    adfFreeDirList(head);
                    return null;
                }

                if (recurs && entry.type == AdfConstants.ST_DIR) {
                    cell.subdir = adfGetRDirEnt(vol, entry.sector, recurs);
                }

                /* same hashcode linked list */
                nextSector = entryBlk.nextSameHash;
                while (nextSector != 0) {
                    entry = new Entry();
                    if (entry == null) {
                        adfFreeDirList(head);
                        if (adfEnv != null && adfEnv.eFct != null) {
                            adfEnv.eFct.call("adfGetDirEnt : malloc");
                        }
                        return null;
                    }
                    if (adfReadEntryBlock(vol, nextSector, entryBlk) != AdfError.RC_OK) {
                        adfFreeDirList(head);
                        return null;
                    }

                    if (adfEntBlock2Entry(entryBlk, entry) != AdfError.RC_OK) {
                        adfFreeDirList(head);
                        return null;
                    }
                    entry.sector = nextSector;

                    cell = newCell(cell, entry);
                    if (cell == null) {
                        adfFreeDirList(head);
                        return null;
                    }

                    if (recurs && entry.type == AdfConstants.ST_DIR) {
                        cell.subdir = adfGetRDirEnt(vol, entry.sector, recurs);
                    }

                    nextSector = entryBlk.nextSameHash;
                }
            }
        }

        /*    if (parent.extension && isDIRCACHE(vol->dosType) )
            adfReadDirCache(vol,parent.extension);
        */
        return head;
    }

    /*
     * adfGetDirEnt
     *
     */

    public static AdfList adfGetDirEnt(Volume vol, int nSect) {
        return adfGetRDirEnt(vol, nSect, false);
    }

    /*
     * adfFreeEntry
     *
     */

    public static void adfFreeEntry(Entry entry) {
        if (entry == null) {
            return;
        }
        /* in Java GC handles name/comment */
    }

    /*
     * adfToRootDir
     *
     */

    public static AdfError adfToRootDir(Volume vol) {
        vol.curDirPtr = vol.rootBlock;

        return AdfError.RC_OK;
    }

    /*
     * adfChangeDir
     *
     */

    public static AdfError adfChangeDir(Volume vol, String name) {
        BEntryBlock entry = new BEntryBlock();
        int nSect = 0;

        if (adfReadEntryBlock(vol, vol.curDirPtr, entry) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }
        nSect = adfNameToEntryBlk(vol, entry.hashTable, name, entry, null);
        /*printf("adfChangeDir=%d\n",nSect);*/
        if (nSect != -1) {
            vol.curDirPtr = nSect;
            /*        if (*adfEnv.useNotify)
                (*adfEnv.notifyFct)(0,ST_ROOT);*/
            return AdfError.RC_OK;
        } else {
            return AdfError.RC_ERROR;
        }
    }

    /*
     * adfParentDir
     *
     */

    public static AdfError adfParentDir(Volume vol) {
        BEntryBlock entry = new BEntryBlock();

        if (vol.curDirPtr != vol.rootBlock) {
            if (adfReadEntryBlock(vol, vol.curDirPtr, entry) != AdfError.RC_OK) {
                return AdfError.RC_ERROR;
            }
            vol.curDirPtr = entry.parent;
        }
        return AdfError.RC_OK;
    }

    /*
     * adfEntBlock2Entry
     *
     */

    public static AdfError adfEntBlock2Entry(BEntryBlock entryBlk, Entry entry) {
        byte[] buf = new byte[AdfConstants.MAXCMMTLEN + 1];
        int len = 0;

        entry.type = entryBlk.secType;
        entry.parent = entryBlk.parent;

        len = AdfConstants.min(entryBlk.nameLen & 0xFF, AdfConstants.MAXNAMELEN);
        byte[] nameCopy = new byte[len];
        System.arraycopy(entryBlk.name, 0, nameCopy, 0, len);
        entry.name = new String(nameCopy);
        if (entry.name == null) {
            return AdfError.RC_MALLOC;
        }
        /*printf("len=%d name=%s parent=%ld\n",entryBlk->nameLen, entry->name,entry->parent );*/
        adfDays2Date(entryBlk.days, entry);
        entry.hour = entryBlk.mins / 60;
        entry.mins = entryBlk.mins % 60;
        entry.secs = entryBlk.ticks / 50;

        entry.access = -1;
        entry.size = 0L;
        entry.comment = null;
        entry.real = 0;
        switch (entryBlk.secType) {
        case AdfConstants.ST_ROOT:
            break;
        case AdfConstants.ST_DIR:
            entry.access = entryBlk.access;
            len = AdfConstants.min(entryBlk.commLen & 0xFF, AdfConstants.MAXCMMTLEN);
            byte[] cmt = new byte[len];
            System.arraycopy(entryBlk.comment, 0, cmt, 0, len);
            entry.comment = new String(cmt);
            if (entry.comment == null) {
                return AdfError.RC_MALLOC;
            }
            break;
        case AdfConstants.ST_FILE:
            entry.access = entryBlk.access;
            entry.size = entryBlk.byteSize & 0xFFFFFFFFL;
            len = AdfConstants.min(entryBlk.commLen & 0xFF, AdfConstants.MAXCMMTLEN);
            byte[] cmt2 = new byte[len];
            System.arraycopy(entryBlk.comment, 0, cmt2, 0, len);
            entry.comment = new String(cmt2);
            if (entry.comment == null) {
                return AdfError.RC_MALLOC;
            }
            break;
        case AdfConstants.ST_LFILE:
        case AdfConstants.ST_LDIR:
            entry.real = entryBlk.realEntry;
        case AdfConstants.ST_LSOFT:
            break;
        default:
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("unknown entry type");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfNameToEntryBlk
     *
     */

    public static int adfNameToEntryBlk(Volume vol, int[] ht, String name,
            BEntryBlock entry, int[] nUpdSect) {
        int hashVal = 0;
        byte[] upperName = new byte[AdfConstants.MAXNAMELEN + 1];
        byte[] upperName2 = new byte[AdfConstants.MAXNAMELEN + 1];
        int nSect = 0;
        int nameLen = 0;
        boolean found = false;
        int updSect = 0;
        boolean intl = false;

        intl = AdfConstants.isINTL(vol.dosType & 0xFF) || AdfConstants.isDIRCACHE(vol.dosType & 0xFF);
        hashVal = adfGetHashValue(name.getBytes(), intl);
        nameLen = AdfConstants.min(name.length(), AdfConstants.MAXNAMELEN);
        myToUpper(upperName, name.getBytes(), nameLen, intl);

        nSect = ht[hashVal];
        /*printf("name=%s ht[%d]=%d upper=%s len=%d\n",name,hashVal,nSect,upperName,nameLen);
        printf("hashVal=%d\n",adfGetHashValue(upperName, intl ));
        if (!strcmp("españa.country",name)) {
        int i;
        for(i=0; i<HT_SIZE; i++) printf("ht[%d]=%d    ",i,ht[i]);
        }*/
        if (nSect == 0) {
            return -1;
        }

        updSect = 0;
        found = false;
        do {
            if (adfReadEntryBlock(vol, nSect, entry) != AdfError.RC_OK) {
                return -1;
            }
            if (nameLen == (entry.nameLen & 0xFF)) {
                myToUpper(upperName2, entry.name, nameLen, intl);
                /*printf("2=%s %s\n",upperName2,upperName);*/
                found = strncmp(upperName, upperName2, nameLen) == 0;
            }
            if (!found) {
                updSect = nSect;
                nSect = entry.nextSameHash;
            }
        } while (!found && nSect != 0);

        if (nSect == 0 && !found) {
            return -1;
        } else {
            if (nUpdSect != null) {
                nUpdSect[0] = updSect;
            }
            return nSect;
        }
    }

    /*
     * Access2String
     *
     */

    public static String adfAccess2String(int acc) {
        char[] ret = new char[8 + 1];
        String s = "----rwed";
        for (int i = 0; i < 8; i++) {
            ret[i] = s.charAt(i);
        }
        if (AdfConstants.hasD(acc)) {
            ret[7] = '-';
        }
        if (AdfConstants.hasE(acc)) {
            ret[6] = '-';
        }
        if (AdfConstants.hasW(acc)) {
            ret[5] = '-';
        }
        if (AdfConstants.hasR(acc)) {
            ret[4] = '-';
        }
        if (AdfConstants.hasA(acc)) {
            ret[3] = 'a';
        }
        if (AdfConstants.hasP(acc)) {
            ret[2] = 'p';
        }
        if (AdfConstants.hasS(acc)) {
            ret[1] = 's';
        }
        if (AdfConstants.hasH(acc)) {
            ret[0] = 'h';
        }
        ret[8] = '\0';
        return new String(ret, 0, 8);
    }

    /*
     * adfCreateEntry
     *
     * if 'thisSect'==-1, allocate a sector, and insert its pointer into the hashTable of 'dir', using the
     * name 'name'. if 'thisSect'!=-1, insert this sector pointer  into the hashTable
     * (here 'thisSect' must be allocated before in the bitmap).
     */

    public static int adfCreateEntry(Volume vol, BEntryBlock dir, String name,
            int thisSect) {
        boolean intl = false;
        BEntryBlock updEntry = new BEntryBlock();
        int len = 0;
        int hashValue = 0;
        AdfError rc = AdfError.RC_OK;
        byte[] name2 = new byte[AdfConstants.MAXNAMELEN + 1];
        byte[] name3 = new byte[AdfConstants.MAXNAMELEN + 1];
        int nSect = 0;
        int newSect = 0;
        int newSect2 = 0;
        BRootBlock root = new BRootBlock();

        /*puts("adfCreateEntry in");*/

        intl = AdfConstants.isINTL(vol.dosType & 0xFF) || AdfConstants.isDIRCACHE(vol.dosType & 0xFF);
        len = AdfConstants.min(name.length(), AdfConstants.MAXNAMELEN);
        myToUpper(name2, name.getBytes(), len, intl);
        hashValue = adfGetHashValue(name.getBytes(), intl);
        nSect = dir.hashTable[hashValue];

        if (nSect == 0) {
            if (thisSect != -1) {
                newSect = thisSect;
            } else {
                newSect = AdfBitm.adfGet1FreeBlock(vol);
                if (newSect == -1) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCreateEntry : nSect==-1");
                    }
                    return -1;
                }
            }

            dir.hashTable[hashValue] = newSect;
            if (dir.secType == AdfConstants.ST_ROOT) {
                /* need to write root block */
                BRootBlock tmpRoot = new BRootBlock();
                if (AdfRaw.adfReadRootBlock(vol, vol.rootBlock, tmpRoot) != AdfError.RC_OK) {
                    /* fallback: try to construct from dir */
                    tmpRoot = entryBlockToRootBlock(dir);
                } else {
                    System.arraycopy(dir.hashTable, 0, tmpRoot.hashTable, 0, AdfConstants.HT_SIZE);
                }
                DateTime dt = adfGiveCurrentTime();
                int[] day = new int[1];
                int[] min = new int[1];
                int[] ticks = new int[1];
                adfTime2AmigaTime(dt, day, min, ticks);
                tmpRoot.cDays = day[0];
                tmpRoot.cMins = min[0];
                tmpRoot.cTicks = ticks[0];
                rc = AdfRaw.adfWriteRootBlock(vol, vol.rootBlock, tmpRoot);
                /* keep dir in sync */
                dir.hashTable[hashValue] = newSect;
            } else {
                DateTime dt = adfGiveCurrentTime();
                int[] day = new int[1];
                int[] min = new int[1];
                int[] ticks = new int[1];
                adfTime2AmigaTime(dt, day, min, ticks);
                dir.days = day[0];
                dir.mins = min[0];
                dir.ticks = ticks[0];
                rc = adfWriteDirBlockFromEntry(vol, dir.headerKey, dir);
            }
            /*puts("adfCreateEntry out, dir");*/
            if (rc != AdfError.RC_OK) {
                AdfBitm.adfSetBlockFree(vol, newSect);
                return -1;
            } else {
                return newSect;
            }
        }

        do {
            if (adfReadEntryBlock(vol, nSect, updEntry) != AdfError.RC_OK) {
                return -1;
            }
            if ((updEntry.nameLen & 0xFF) == len) {
                myToUpper(name3, updEntry.name, updEntry.nameLen & 0xFF, intl);
                if (strncmp(name3, name2, len) == 0) {
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfCreateEntry : entry already exists");
                    }
                    return -1;
                }
            }
            nSect = updEntry.nextSameHash;
        } while (nSect != 0);

        if (thisSect != -1) {
            newSect2 = thisSect;
        } else {
            newSect2 = AdfBitm.adfGet1FreeBlock(vol);
            if (newSect2 == -1) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfCreateEntry : nSect==-1");
                }
                return -1;
            }
        }

        rc = AdfError.RC_OK;
        updEntry.nextSameHash = newSect2;
        if (updEntry.secType == AdfConstants.ST_DIR) {
            rc = adfWriteDirBlockFromEntry(vol, updEntry.headerKey, updEntry);
        } else if (updEntry.secType == AdfConstants.ST_FILE) {
            rc = AdfFile.adfWriteFileHdrBlock(vol, updEntry.headerKey,
                    entryBlockToFileHeader(updEntry));
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCreateEntry : unknown entry type");
            }
        }

        /*puts("adfCreateEntry out, hash");*/
        if (rc != AdfError.RC_OK) {
            AdfBitm.adfSetBlockFree(vol, newSect2);
            return -1;
        } else {
            return newSect2;
        }
    }

    /*
     * adfIntlToUpper
     *
     */

    public static int adfIntlToUpper(int c) {
        c &= 0xFF;
        return (c >= 'a' && c <= 'z') || (c >= 224 && c <= 254 && c != 247) ? c - ('a' - 'A') : c;
    }

    public static int adfToUpper(int c) {
        c &= 0xFF;
        return (c >= 'a' && c <= 'z') ? c - ('a' - 'A') : c;
    }

    /*
     * myToUpper
     *
     */

    public static void myToUpper(byte[] nstr, byte[] ostr, int nlen, boolean intl) {
        int i = 0;

        if (intl) {
            for (i = 0; i < nlen; i++) {
                nstr[i] = (byte) adfIntlToUpper(ostr[i] & 0xFF);
            }
        } else {
            for (i = 0; i < nlen; i++) {
                nstr[i] = (byte) adfToUpper(ostr[i] & 0xFF);
            }
        }
        nstr[nlen] = 0;
    }

    /*
     * adfGetHashValue
     * 
     */

    public static int adfGetHashValue(byte[] name, boolean intl) {
        long hash = 0;
        int len = 0;
        int i = 0;
        int upper = 0;
        /* find strlen up to null */
        int namelen = 0;
        for (namelen = 0; namelen < name.length; namelen++) {
            if (name[namelen] == 0) {
                break;
            }
        }
        /* if no null, use full length but capped? C uses strlen on char* */
        if (namelen == name.length) {
            /* fallback: use string length */
            namelen = name.length;
        }
        /* For String callers we pass getBytes without null, so namelen==length */
        len = namelen;
        /* But original C does: len = hash = strlen((char*)name); */
        /* If name came from String.getBytes without null, strlen would be length */
        /* Use decoding of name bytes as string for len if needed */
        /* Recompute if name was from String: use whole array */
        /* To match C exactly for Java String case, caller should pass bytes with null? */
        /* Instead, if name has trailing zeros, strlen works; otherwise len = name.length */
        hash = len;
        for (i = 0; i < len; i++) {
            if (intl) {
                upper = adfIntlToUpper(name[i] & 0xFF);
            } else {
                upper = toUpper(name[i] & 0xFF);
            }
            hash = (hash * 13 + upper) & 0x7FF;
        }
        hash = hash % AdfConstants.HT_SIZE;

        return (int) hash;
    }

    /** String overload — converts to bytes then delegates. */
    public static int adfGetHashValue(String nameStr, boolean intl) {
        return adfGetHashValue(nameStr.getBytes(), intl);
    }

    /** Helper for adfGetHashValue — mirrors toupper for non-intl. */
    private static int toUpper(int c) {
        c &= 0xFF;
        if (c >= 'a' && c <= 'z') {
            return c - 32;
        }
        return c;
    }

    /*
     * printEntry
     *
     */

    public static void printEntry(Entry entry) {
        System.out.printf("%-30s %2d %6d ", entry.name, entry.type, entry.sector);
        System.out.printf("%2d/%02d/%04d %2d:%02d:%02d", entry.days, entry.month, entry.year,
                entry.hour, entry.mins, entry.secs);
        if (entry.type == AdfConstants.ST_FILE) {
            System.out.printf("%8d ", entry.size);
        } else {
            System.out.printf("         ");
        }
        if (entry.type == AdfConstants.ST_FILE || entry.type == AdfConstants.ST_DIR) {
            System.out.printf("%s ", adfAccess2String(entry.access));
        }
        if (entry.comment != null) {
            System.out.printf("%s ", entry.comment);
        }
        System.out.printf("\n");
    }

    /*
     * adfCreateDir
     *
     */

    public static AdfError adfCreateDir(Volume vol, int nParent, String name) {
        int nSect = 0;
        BDirBlock dir = new BDirBlock();
        BEntryBlock parent = new BEntryBlock();

        if (adfReadEntryBlock(vol, nParent, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        /* -1 : do not use a specific, already allocated sector */
        nSect = adfCreateEntry(vol, parent, name, -1);
        if (nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCreateDir : no sector available");
            }
            return AdfError.RC_ERROR;
        }
        /* memset(&dir, 0, sizeof(struct bDirBlock)); */
        dir = new BDirBlock();
        dir.nameLen = (byte) AdfConstants.min(AdfConstants.MAXNAMELEN, name.length());
        byte[] nameBytes = name.getBytes();
        for (int i = 0; i < (dir.nameLen & 0xFF); i++) {
            dir.dirName[i] = nameBytes[i];
        }
        dir.headerKey = nSect;

        if (parent.secType == AdfConstants.ST_ROOT) {
            dir.parent = vol.rootBlock;
        } else {
            dir.parent = parent.headerKey;
        }
        DateTime dt = adfGiveCurrentTime();
        int[] day = new int[1];
        int[] min = new int[1];
        int[] ticks = new int[1];
        adfTime2AmigaTime(dt, day, min, ticks);
        dir.days = day[0];
        dir.mins = min[0];
        dir.ticks = ticks[0];

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            /* for adfCreateEmptyCache, will be added by adfWriteDirBlock */
            dir.secType = AdfConstants.ST_DIR;
            BEntryBlock dirAsEntry = dirBlockToEntryBlock(dir);
            AdfCache.adfAddInCache(vol, parent, dirAsEntry);
            AdfCache.adfCreateEmptyCache(vol, dirAsEntry, -1);
        }

        /* writes the dirblock, with the possible dircache assiocated */
        if (adfWriteDirBlock(vol, nSect, dir) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        AdfBitm.adfUpdateBitmap(vol);

        if (adfEnv != null && adfEnv.useNotify && adfEnv.notifyFct != null) {
            adfEnv.notifyFct.notify(nParent, AdfConstants.ST_DIR);
        }

        return AdfError.RC_OK;
    }

    /*
     * adfCreateFile
     *
     */

    public static AdfError adfCreateFile(Volume vol, int nParent, String name,
            BFileHeaderBlock fhdr) {
        int nSect = 0;
        BEntryBlock parent = new BEntryBlock();
        /*puts("adfCreateFile in");*/
        if (adfReadEntryBlock(vol, nParent, parent) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        /* -1 : do not use a specific, already allocated sector */
        nSect = adfCreateEntry(vol, parent, name, -1);
        if (nSect == -1) {
            return AdfError.RC_ERROR;
        }
        /*printf("new fhdr=%d\n",nSect);*/
        /* memset(fhdr,0,512); */
        BFileHeaderBlock tmp = new BFileHeaderBlock();
        fhdr.type = tmp.type;
        fhdr.headerKey = tmp.headerKey;
        fhdr.highSeq = tmp.highSeq;
        fhdr.dataSize = tmp.dataSize;
        fhdr.firstData = tmp.firstData;
        fhdr.checkSum = tmp.checkSum;
        Arrays.fill(fhdr.dataBlocks, 0);
        fhdr.r1 = 0;
        fhdr.r2 = 0;
        fhdr.access = 0;
        fhdr.byteSize = 0;
        fhdr.commLen = 0;
        Arrays.fill(fhdr.comment, (byte) 0);
        Arrays.fill(fhdr.r3, (byte) 0);
        fhdr.days = 0;
        fhdr.mins = 0;
        fhdr.ticks = 0;
        fhdr.nameLen = 0;
        Arrays.fill(fhdr.fileName, (byte) 0);
        fhdr.r4 = 0;
        fhdr.real = 0;
        fhdr.nextLink = 0;
        Arrays.fill(fhdr.r5, 0);
        fhdr.nextSameHash = 0;
        fhdr.parent = 0;
        fhdr.extension = 0;
        fhdr.secType = 0;
        fhdr.nameLen = (byte) AdfConstants.min(AdfConstants.MAXNAMELEN, name.length());
        byte[] nameBytes = name.getBytes();
        for (int i = 0; i < (fhdr.nameLen & 0xFF); i++) {
            fhdr.fileName[i] = nameBytes[i];
        }
        fhdr.headerKey = nSect;
        if (parent.secType == AdfConstants.ST_ROOT) {
            fhdr.parent = vol.rootBlock;
        } else if (parent.secType == AdfConstants.ST_DIR) {
            fhdr.parent = parent.headerKey;
        } else {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfCreateFile : unknown parent secType");
            }
        }
        DateTime dt = adfGiveCurrentTime();
        int[] day = new int[1];
        int[] min = new int[1];
        int[] ticks = new int[1];
        adfTime2AmigaTime(dt, day, min, ticks);
        fhdr.days = day[0];
        fhdr.mins = min[0];
        fhdr.ticks = ticks[0];

        if (AdfFile.adfWriteFileHdrBlock(vol, nSect, fhdr) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        if (AdfConstants.isDIRCACHE(vol.dosType & 0xFF)) {
            BEntryBlock fhdrAsEntry = fileHeaderToEntryBlock(fhdr);
            AdfCache.adfAddInCache(vol, parent, fhdrAsEntry);
        }

        AdfBitm.adfUpdateBitmap(vol);

        if (adfEnv != null && adfEnv.useNotify && adfEnv.notifyFct != null) {
            adfEnv.notifyFct.notify(nParent, AdfConstants.ST_FILE);
        }

        return AdfError.RC_OK;
    }

    /*
     * adfReadEntryBlock
     *
     */

    public static AdfError adfReadEntryBlock(Volume vol, int nSect, BEntryBlock ent) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];

        if (AdfDisk.adfReadBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BEntryBlock tmp = BEntryBlock.read(bb, 0);
        /* memcpy(ent, buf, 512); */
        copyEntryBlock(tmp, ent);
        /* swapEndian handled by read */
        /*printf("readentry=%d\n",nSect);*/
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
        if ((ent.nameLen & 0xFF) < 0 || (ent.nameLen & 0xFF) > AdfConstants.MAXNAMELEN
                || (ent.commLen & 0xFF) > AdfConstants.MAXCMMTLEN) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadEntryBlock : nameLen or commLen incorrect");
            }
            System.out.printf("nameLen=%d, commLen=%d, name=%s sector%d\n",
                    ent.nameLen & 0xFF, ent.commLen & 0xFF, new String(ent.name, 0, ent.nameLen & 0xFF), ent.headerKey);
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteEntryBlock
     *
     */

    public static AdfError adfWriteEntryBlock(Volume vol, int nSect, BEntryBlock ent) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        ent.write(bb, 0);

        newSum = AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE);
        swLong(buf, 20, newSum);

        if (AdfDisk.adfWriteBlock(vol, nSect, buf) != AdfError.RC_OK) {
            return AdfError.RC_ERROR;
        }

        return AdfError.RC_OK;
    }

    /*
     * adfWriteDirBlock
     *
     */

    public static AdfError adfWriteDirBlock(Volume vol, int nSect, BDirBlock dir) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;

        /*printf("wdirblk=%d\n",nSect);*/
        dir.type = AdfConstants.T_HEADER;
        dir.highSeq = 0;
        dir.hashTableSize = 0;
        dir.secType = AdfConstants.ST_DIR;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        dir.write(bb, 0);
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

    private static void adfDays2Date(int days, Entry entry) {
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

        entry.year = y;
        entry.month = m;
        entry.days = days + 1;
    }

    private static boolean adfIsLeap(int y) {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
    }

    private static DateTime adfGiveCurrentTime() {
        java.util.Calendar cal = java.util.Calendar.getInstance();
        DateTime dt = new DateTime();
        dt.year = cal.get(java.util.Calendar.YEAR) - 1900;
        dt.mon = cal.get(java.util.Calendar.MONTH) + 1;
        dt.day = cal.get(java.util.Calendar.DAY_OF_MONTH);
        dt.hour = cal.get(java.util.Calendar.HOUR_OF_DAY);
        dt.min = cal.get(java.util.Calendar.MINUTE);
        dt.sec = cal.get(java.util.Calendar.SECOND);
        return dt;
    }

    private static void adfTime2AmigaTime(DateTime dt, int[] day, int[] min, int[] ticks) {
        int[] jm = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

        min[0] = dt.hour * 60 + dt.min;
        ticks[0] = dt.sec * 50;

        day[0] = dt.day - 1;

        if (dt.mon > 1) {
            int mon = dt.mon - 1;
            if (mon > 2 && adfIsLeap(dt.year)) {
                jm[1] = 29;
            }
            while (mon > 0) {
                day[0] = day[0] + jm[mon - 1];
                mon--;
            }
        }

        if (dt.year > 78) {
            int year = dt.year - 1;
            while (year >= 78) {
                if (adfIsLeap(year)) {
                    day[0] = day[0] + 366;
                } else {
                    day[0] = day[0] + 365;
                }
                year--;
            }
        }
    }

    private static int strncmp(byte[] a, byte[] b, int n) {
        for (int i = 0; i < n; i++) {
            int ca = a[i] & 0xFF;
            int cb = b[i] & 0xFF;
            if (ca != cb) {
                return ca - cb;
            }
            if (ca == 0) {
                break;
            }
        }
        return 0;
    }

    private static void copyEntryBlock(BEntryBlock src, BEntryBlock dst) {
        dst.type = src.type;
        dst.headerKey = src.headerKey;
        System.arraycopy(src.r1, 0, dst.r1, 0, src.r1.length);
        dst.checkSum = src.checkSum;
        System.arraycopy(src.hashTable, 0, dst.hashTable, 0, src.hashTable.length);
        System.arraycopy(src.r2, 0, dst.r2, 0, src.r2.length);
        dst.access = src.access;
        dst.byteSize = src.byteSize;
        dst.commLen = src.commLen;
        System.arraycopy(src.comment, 0, dst.comment, 0, src.comment.length);
        System.arraycopy(src.r3, 0, dst.r3, 0, src.r3.length);
        dst.days = src.days;
        dst.mins = src.mins;
        dst.ticks = src.ticks;
        dst.nameLen = src.nameLen;
        System.arraycopy(src.name, 0, dst.name, 0, src.name.length);
        dst.r4 = src.r4;
        dst.realEntry = src.realEntry;
        dst.nextLink = src.nextLink;
        System.arraycopy(src.r5, 0, dst.r5, 0, src.r5.length);
        dst.nextSameHash = src.nextSameHash;
        dst.parent = src.parent;
        dst.extension = src.extension;
        dst.secType = src.secType;
    }

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
        /* r5 */
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
        /* dataBlocks overlap hashTable */
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

    private static BRootBlock entryBlockToRootBlock(BEntryBlock e) {
        BRootBlock r = new BRootBlock();
        r.type = e.type;
        r.headerKey = e.headerKey;
        r.highSeq = e.r1[0];
        r.hashTableSize = e.r1[1];
        r.firstData = e.r1[2];
        r.checkSum = e.checkSum;
        System.arraycopy(e.hashTable, 0, r.hashTable, 0, e.hashTable.length);
        r.bmFlag = e.r2[0];
        /* bmPages from hashTable tail? Not accurate, but fallback */
        r.bmExt = e.r2[1];
        r.days = e.days;
        r.mins = e.mins;
        r.ticks = e.ticks;
        r.nameLen = e.nameLen;
        System.arraycopy(e.name, 0, r.diskName, 0, Math.min(e.name.length, r.diskName.length));
        r.nextSameHash = e.nextSameHash;
        r.parent = e.parent;
        r.extension = e.extension;
        r.secType = e.secType;
        return r;
    }

    private static AdfError adfWriteDirBlockFromEntry(Volume vol, int nSect, BEntryBlock ent) {
        BDirBlock dir = entryBlockToDirBlock(ent);
        return adfWriteDirBlock(vol, nSect, dir);
    }

    private static AdfError adfWriteRootBlockFromEntry(Volume vol, int nSect, BEntryBlock ent) {
        BRootBlock root = new BRootBlock();
        if (AdfRaw.adfReadRootBlock(vol, nSect, root) != AdfError.RC_OK) {
            root = entryBlockToRootBlock(ent);
            System.arraycopy(ent.hashTable, 0, root.hashTable, 0, AdfConstants.HT_SIZE);
            return AdfRaw.adfWriteRootBlock(vol, nSect, root);
        }
        System.arraycopy(ent.hashTable, 0, root.hashTable, 0, AdfConstants.HT_SIZE);
        /* also sync parent/extension if needed */
        root.extension = ent.extension;
        root.secType = ent.secType;
        return AdfRaw.adfWriteRootBlock(vol, nSect, root);
    }
}
