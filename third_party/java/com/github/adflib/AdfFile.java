/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_file.c / adf_file.h
 *
 *  $Id$
 *
 *  file code
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
 * Java port of {@code adf_file.c} / {@code adf_file.h}.
 *
 * <p>Keeps original C control flow and helper naming. ByteBuffer I/O uses
 * absolute {@code get(int)}/{@code put(int,byte)} without touching
 * {@code position}/{@code limit} and honours {@code BIG_ENDIAN} for ADF data.
 * High-level objects use normal Java classes ({@link File}, {@link FileBlocks}).
 * Return codes use {@link AdfError} with out-parameters as single-element arrays.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfFile {

    private AdfFile() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    public static void adfFileTruncate(Volume vol, int nParent, String name) {

    }

    /*
     * adfFileFlush
     *
     */

    public static void adfFlushFile(File file) {
        BEntryBlock parent = new BEntryBlock();
        BOFSDataBlock data = null;

        if (file.currentExt != null) {
            if (file.writeMode) {
                adfWriteFileExtBlock(file.volume, file.currentExt.headerKey,
                        file.currentExt);
            }
        }
        if (file.currentData != null) {
            if (file.writeMode) {
                file.fileHdr.byteSize = file.pos & 0xFFFFFFFFL;
                if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
                    /* data is BOFSDataBlock view of currentData */
                    ByteBuffer bb = file.currentData.duplicate().order(ByteOrder.BIG_ENDIAN);
                    BOFSDataBlock d = BOFSDataBlock.read(bb, 0);
                    d.dataSize = file.posInDataBlk;
                    /* write back dataSize into buffer for later write */
                    ByteBuffer wb = ByteBuffer.allocate(512).order(ByteOrder.BIG_ENDIAN);
                    d.write(wb, 0);
                    /* copy wb into file.currentData without touching position */
                    for (int i = 0; i < 512; i++) {
                        file.currentData.put(i, wb.get(i));
                    }
                }
                if (file.fileHdr.byteSize > 0) {
                    adfWriteDataBlock(file.volume, file.curDataPtr,
                            file.currentData);
                }
            }
        }
        if (file.writeMode) {
            file.fileHdr.byteSize = file.pos & 0xFFFFFFFFL;
            /*printf("pos=%ld\n",file->pos);*/
            DateTime dt = adfGiveCurrentTime();
            int[] day = new int[1];
            int[] min = new int[1];
            int[] ticks = new int[1];
            adfTime2AmigaTime(dt, day, min, ticks);
            file.fileHdr.days = day[0];
            file.fileHdr.mins = min[0];
            file.fileHdr.ticks = ticks[0];
            adfWriteFileHdrBlock(file.volume, file.fileHdr.headerKey, file.fileHdr);

            if (AdfConstants.isDIRCACHE(file.volume.dosType & 0xFF)) {
                /*printf("parent=%ld\n",file->fileHdr->parent);*/
                AdfDir.adfReadEntryBlock(file.volume, file.fileHdr.parent, parent);
                AdfCache.adfUpdateCache(file.volume, parent, fileHeaderToEntryBlock(file.fileHdr), false);
            }
            AdfBitm.adfUpdateBitmap(file.volume);
        }
    }

    /*
     * adfGetFileBlocks
     *
     */

    public static AdfError adfGetFileBlocks(Volume vol, BFileHeaderBlock entry,
            FileBlocks fileBlocks) {
        int n = 0;
        int m = 0;
        int nSect = 0;
        BFileExtBlock extBlock = new BFileExtBlock();
        int i = 0;

        fileBlocks.header = entry.headerKey;
        int[] nbDataArr = new int[1];
        int[] nbExtArr = new int[1];
        adfFileRealSize(entry.byteSize, vol.datablockSize,
                nbDataArr, nbExtArr);
        fileBlocks.nbData = nbDataArr[0];
        fileBlocks.nbExtens = nbExtArr[0];

        fileBlocks.data = new int[fileBlocks.nbData];
        if (fileBlocks.data == null) {
            if (adfEnv != null && adfEnv.eFct != null) {
                adfEnv.eFct.call("adfGetFileBlocks : malloc");
            }
            return AdfError.RC_MALLOC;
        }

        fileBlocks.extens = new int[fileBlocks.nbExtens];
        if (fileBlocks.extens == null) {
            if (adfEnv != null && adfEnv.eFct != null) {
                adfEnv.eFct.call("adfGetFileBlocks : malloc");
            }
            return AdfError.RC_MALLOC;
        }

        n = 0;
        m = 0;
        /* in file header block */
        for (i = 0; i < entry.highSeq; i++) {
            fileBlocks.data[n++] = entry.dataBlocks[AdfConstants.MAX_DATABLK - 1 - i];
        }

        /* in file extension blocks */
        nSect = entry.extension;
        while (nSect != 0) {
            fileBlocks.extens[m++] = nSect;
            adfReadFileExtBlock(vol, nSect, extBlock);
            for (i = 0; i < extBlock.highSeq; i++) {
                fileBlocks.data[n++] = extBlock.dataBlocks[AdfConstants.MAX_DATABLK - 1 - i];
            }
            nSect = extBlock.extension;
        }
        if ((fileBlocks.nbExtens + fileBlocks.nbData) != (n + m)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfGetFileBlocks : less blocks than expected");
            }
        }

        return AdfError.RC_OK;
    }

    /*
     * adfFreeFileBlocks
     *
     */

    public static AdfError adfFreeFileBlocks(Volume vol, BFileHeaderBlock entry) {
        int i = 0;
        FileBlocks fileBlocks = new FileBlocks();
        AdfError rc = AdfError.RC_OK;

        adfGetFileBlocks(vol, entry, fileBlocks);

        for (i = 0; i < fileBlocks.nbData; i++) {
            AdfBitm.adfSetBlockFree(vol, fileBlocks.data[i]);
        }
        for (i = 0; i < fileBlocks.nbExtens; i++) {
            AdfBitm.adfSetBlockFree(vol, fileBlocks.extens[i]);
        }

        /* free(fileBlocks.data); free(fileBlocks.extens); */

        return rc;
    }

    /*
     * adfFileRealSize
     *
     * Compute and return real number of block used by one file
     * Compute number of datablocks and file extension blocks
     *
     */

    public static int adfFileRealSize(long size, int blockSize, int[] dataN, int[] extN) {
        int data = 0;
        int ext = 0;

        /*--- number of data blocks ---*/
        data = (int) (size / blockSize);
        if (size % blockSize != 0) {
            data++;
        }

        /*--- number of header extension blocks ---*/
        ext = 0;
        if (data > AdfConstants.MAX_DATABLK) {
            ext = (data - AdfConstants.MAX_DATABLK) / AdfConstants.MAX_DATABLK;
            if ((data - AdfConstants.MAX_DATABLK) % AdfConstants.MAX_DATABLK != 0) {
                ext++;
            }
        }

        if (dataN != null) {
            dataN[0] = data;
        }
        if (extN != null) {
            extN[0] = ext;
        }

        return ext + data + 1;
    }

    /*
     * adfWriteFileHdrBlock
     *
     */

    public static AdfError adfWriteFileHdrBlock(Volume vol, int nSect, BFileHeaderBlock fhdr) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;
        AdfError rc = AdfError.RC_OK;
        /*printf("adfWriteFileHdrBlock %ld\n",nSect);*/
        fhdr.type = AdfConstants.T_HEADER;
        fhdr.dataSize = 0;
        fhdr.secType = AdfConstants.ST_FILE;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        fhdr.write(bb, 0);
        newSum = AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE);
        swLong(buf, 20, newSum);
        /*    *(uint32_t*)(buf+20) = swapLong((uint8_t*)&newSum);*/

        AdfDisk.adfWriteBlock(vol, nSect, buf);

        return rc;
    }

    /*
     * adfFileSeek
     *
     */

    public static void adfFileSeek(File file, long pos) {
        int extBlock = 0;
        int nSect = 0;
        long nPos = 0;
        int i = 0;

        nPos = Math.min(pos & 0xFFFFFFFFL, file.fileHdr.byteSize & 0xFFFFFFFFL);
        file.pos = nPos;
        int[] posInExtBlkArr = new int[1];
        int[] posInDataBlkArr = new int[1];
        int[] curDataNArr = new int[1];
        extBlock = adfPos2DataBlock((int) nPos, file.volume.datablockSize,
                posInExtBlkArr, posInDataBlkArr, curDataNArr);
        file.posInExtBlk = posInExtBlkArr[0];
        file.posInDataBlk = posInDataBlkArr[0];
        file.curDataPtr = curDataNArr[0];
        if (extBlock == -1) {
            adfReadDataBlock(file.volume,
                    file.fileHdr.dataBlocks[AdfConstants.MAX_DATABLK - 1 - file.curDataPtr],
                    file.currentData);
        } else {
            nSect = file.fileHdr.extension;
            i = 0;
            while (i < extBlock && nSect != 0) {
                adfReadFileExtBlock(file.volume, nSect, file.currentExt);
                nSect = file.currentExt.extension;
                i++;
            }
            if (i != extBlock) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("error");
                }
            }
            adfReadDataBlock(file.volume,
                    file.currentExt.dataBlocks[file.posInExtBlk], file.currentData);
        }
    }

    /*
     * adfFileOpen
     *
     */

    public static File adfOpenFile(Volume vol, String name, String mode) {
        File file = null;
        int nSect = 0;
        BEntryBlock entry = new BEntryBlock();
        BEntryBlock parent = new BEntryBlock();
        boolean write = false;

        write = ("w".equals(mode) || "a".equals(mode));

        if (write && vol.dev.readOnly) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : device is mounted 'read only'");
            }
            return null;
        }

        AdfDir.adfReadEntryBlock(vol, vol.curDirPtr, parent);

        nSect = AdfDir.adfNameToEntryBlk(vol, parent.hashTable, name, entry, null);
        if (!write && nSect == -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : file \"" + name + "\" not found.");
            }
            /*fprintf(stdout,"filename %s %d, parent =%d\n",name,strlen(name),vol->curDirPtr);*/
            return null;
        }
        if (!write && AdfConstants.hasR(entry.access)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : access denied");
            }
            return null;
        }
        /*    if (entry.secType!=ST_FILE) {
            (*adfEnv.wFct)("adfFileOpen : not a file"); return NULL; }
        if (write && (hasE(entry.access)||hasW(entry.access))) {
            (*adfEnv.wFct)("adfFileOpen : access denied"); return NULL; }
        */ if (write && nSect != -1) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : file already exists");
            }
            return null;
        }

        file = new File();
        if (file == null) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : malloc");
            }
            return null;
        }
        file.fileHdr = new BFileHeaderBlock();
        if (file.fileHdr == null) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : malloc");
            }
            return null;
        }
        file.currentData = ByteBuffer.allocate(512).order(ByteOrder.BIG_ENDIAN);
        if (file.currentData == null) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfFileOpen : malloc");
            }
            return null;
        }

        file.volume = vol;
        file.pos = 0;
        file.posInExtBlk = 0;
        file.posInDataBlk = 0;
        file.writeMode = write;
        file.currentExt = null;
        file.nDataBlock = 0;

        if ("w".equals(mode)) {
            /* memset(file->fileHdr,0,512); */
            file.fileHdr = new BFileHeaderBlock();
            AdfDir.adfCreateFile(vol, vol.curDirPtr, name, file.fileHdr);
            file.eof = true;
        } else if ("a".equals(mode)) {
            /* memcpy(file->fileHdr,&entry,sizeof(struct bFileHeaderBlock)); */
            entryBlockToFileHeader(entry, file.fileHdr);
            file.eof = true;
            adfFileSeek(file, file.fileHdr.byteSize);
        } else if ("r".equals(mode)) {
            entryBlockToFileHeader(entry, file.fileHdr);
            file.eof = false;
        }

        /*puts("adfOpenFile");*/
        return file;
    }

    /*
     * adfCloseFile
     *
     */

    public static void adfCloseFile(File file) {

        if (file == null) {
            return;
        }
        /*puts("adfCloseFile in");*/

        adfFlushFile(file);

        /* if (file->currentExt) free(file->currentExt); */

        /* if (file->currentData) free(file->currentData); */

        /* free(file->fileHdr); free(file); */

        /*puts("adfCloseFile out");*/
    }

    /*
     * adfReadFile
     *
     */

    public static int adfReadFile(File file, int n, ByteBuffer buffer) {
        int bytesRead = 0;
        byte[] dataPtr = null;
        /* dataPtr offset handling via ByteBuffer */
        int blockSize = 0;
        int size = 0;

        if (n == 0) {
            return n;
        }
        blockSize = file.volume.datablockSize;
        /*puts("adfReadFile");*/
        if (file.pos + n > (file.fileHdr.byteSize & 0xFFFFFFFFL)) {
            n = (int) ((file.fileHdr.byteSize & 0xFFFFFFFFL) - file.pos);
        }

        int dataOffset = 0;
        if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            dataOffset = 24;
        } else {
            dataOffset = 0;
        }

        if (file.pos == 0 || file.posInDataBlk == blockSize) {
            adfReadNextFileBlock(file);
            file.posInDataBlk = 0;
        }

        bytesRead = 0;
        size = 0;
        while (bytesRead < n) {
            size = AdfConstants.min(n - bytesRead, blockSize - file.posInDataBlk);
            /* memcpy(bufPtr, dataPtr+file->posInDataBlk, size); */
            for (int i = 0; i < size; i++) {
                buffer.put(bytesRead + i, file.currentData.get(dataOffset + file.posInDataBlk + i));
            }
            file.pos += size;
            bytesRead += size;
            file.posInDataBlk += size;
            if (file.posInDataBlk == blockSize && bytesRead < n) {
                adfReadNextFileBlock(file);
                file.posInDataBlk = 0;
            }
        }
        file.eof = (file.pos == (file.fileHdr.byteSize & 0xFFFFFFFFL));
        return bytesRead;
    }

    /*
     * adfEndOfFile
     *
     */

    public static boolean adfEndOfFile(File file) {
        return file.eof;
    }

    /*
     * adfReadNextFileBlock
     *
     */

    public static AdfError adfReadNextFileBlock(File file) {
        int nSect = 0;
        BOFSDataBlock data = null;
        AdfError rc = AdfError.RC_OK;

        if (file.currentData != null && AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            ByteBuffer bb = file.currentData.duplicate().order(ByteOrder.BIG_ENDIAN);
            data = BOFSDataBlock.read(bb, 0);
        }

        if (file.nDataBlock == 0) {
            nSect = file.fileHdr.firstData;
        } else if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            nSect = data.nextData;
        } else {
            if (file.nDataBlock < AdfConstants.MAX_DATABLK) {
                nSect = file.fileHdr.dataBlocks[AdfConstants.MAX_DATABLK - 1 - file.nDataBlock];
            } else {
                if (file.nDataBlock == AdfConstants.MAX_DATABLK) {
                    file.currentExt = new BFileExtBlock();
                    if (file.currentExt == null) {
                        if (adfEnv != null && adfEnv.eFct != null) {
                            adfEnv.eFct.call("adfReadNextFileBlock : malloc");
                        }
                    }
                    adfReadFileExtBlock(file.volume, file.fileHdr.extension,
                            file.currentExt);
                    file.posInExtBlk = 0;
                } else if (file.posInExtBlk == AdfConstants.MAX_DATABLK) {
                    adfReadFileExtBlock(file.volume, file.currentExt.extension,
                            file.currentExt);
                    file.posInExtBlk = 0;
                }
                nSect = file.currentExt.dataBlocks[AdfConstants.MAX_DATABLK - 1 - file.posInExtBlk];
                file.posInExtBlk++;
            }
        }
        adfReadDataBlock(file.volume, nSect, file.currentData);

        if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            ByteBuffer bb = file.currentData.duplicate().order(ByteOrder.BIG_ENDIAN);
            data = BOFSDataBlock.read(bb, 0);
            if (data.seqNum != file.nDataBlock + 1) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadNextFileBlock : seqnum incorrect");
                }
            }
        }

        file.nDataBlock++;

        return rc;
    }

    /*
     * adfWriteFile
     *
     */

    public static int adfWriteFile(File file, int n, ByteBuffer buffer) {
        int bytesWritten = 0;
        byte[] dataPtr = null;
        int size = 0;
        int blockSize = 0;
        BOFSDataBlock dataB = null;

        bytesWritten = 0;
        if (n == 0) {
            return n;
        }
        /*puts("adfWriteFile");*/
        blockSize = file.volume.datablockSize;
        int dataOffset = 0;
        if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            dataOffset = 24;
        } else {
            dataOffset = 0;
        }

        if (file.pos == 0 || file.posInDataBlk == blockSize) {
            if (adfCreateNextFileBlock(file) == -1) {
                /* bug found by Rikard */
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfWritefile : no more free sector availbale");
                }
                return bytesWritten;
            }
            file.posInDataBlk = 0;
        }

        bytesWritten = 0;
        while (bytesWritten < n) {
            size = AdfConstants.min(n - bytesWritten, blockSize - file.posInDataBlk);
            /* memcpy(dataPtr+file->posInDataBlk, bufPtr, size); */
            for (int i = 0; i < size; i++) {
                file.currentData.put(dataOffset + file.posInDataBlk + i, buffer.get(bytesWritten + i));
            }
            file.pos += size;
            bytesWritten += size;
            file.posInDataBlk += size;
            if (file.posInDataBlk == blockSize && bytesWritten < n) {
                if (adfCreateNextFileBlock(file) == -1) {
                    /* bug found by Rikard */
                    if (adfEnv != null && adfEnv.wFct != null) {
                        adfEnv.wFct.call("adfWritefile : no more free sector availbale");
                    }
                    return bytesWritten;
                }
                file.posInDataBlk = 0;
            }
        }
        return bytesWritten;
    }

    /*
     * adfCreateNextFileBlock
     *
     */

    public static int adfCreateNextFileBlock(File file) {
        int nSect = 0;
        int extSect = 0;
        BOFSDataBlock data = null;
        int blockSize = 0;
        int i = 0;
        /*puts("adfCreateNextFileBlock");*/
        blockSize = file.volume.datablockSize;
        if (file.currentData != null && AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            ByteBuffer bb = file.currentData.duplicate().order(ByteOrder.BIG_ENDIAN);
            data = BOFSDataBlock.read(bb, 0);
        }

        /* the first data blocks pointers are inside the file header block */
        if (file.nDataBlock < AdfConstants.MAX_DATABLK) {
            nSect = AdfBitm.adfGet1FreeBlock(file.volume);
            if (nSect == -1) {
                return -1;
            }
            /*printf("adfCreateNextFileBlock fhdr %ld\n",nSect);*/
            if (file.nDataBlock == 0) {
                file.fileHdr.firstData = nSect;
            }
            file.fileHdr.dataBlocks[AdfConstants.MAX_DATABLK - 1 - file.nDataBlock] = nSect;
            file.fileHdr.highSeq++;
        } else {
            /* one more sector is needed for one file extension block */
            if ((file.nDataBlock % AdfConstants.MAX_DATABLK) == 0) {
                extSect = AdfBitm.adfGet1FreeBlock(file.volume);
                /*printf("extSect=%ld\n",extSect);*/
                if (extSect == -1) {
                    return -1;
                }

                /* the future block is the first file extension block */
                if (file.nDataBlock == AdfConstants.MAX_DATABLK) {
                    file.currentExt = new BFileExtBlock();
                    if (file.currentExt == null) {
                        AdfBitm.adfSetBlockFree(file.volume, extSect);
                        if (adfEnv != null && adfEnv.eFct != null) {
                            adfEnv.eFct.call("adfCreateNextFileBlock : malloc");
                        }
                        return -1;
                    }
                    file.fileHdr.extension = extSect;
                }

                /* not the first : save the current one, and link it with the future */
                if (file.nDataBlock >= 2 * AdfConstants.MAX_DATABLK) {
                    file.currentExt.extension = extSect;
                    /*printf ("write ext=%d\n",file->currentExt->headerKey);*/
                    adfWriteFileExtBlock(file.volume, file.currentExt.headerKey,
                            file.currentExt);
                }

                /* initializes a file extension block */
                for (i = 0; i < AdfConstants.MAX_DATABLK; i++) {
                    file.currentExt.dataBlocks[i] = 0;
                }
                file.currentExt.headerKey = extSect;
                file.currentExt.parent = file.fileHdr.headerKey;
                file.currentExt.highSeq = 0;
                file.currentExt.extension = 0;
                file.posInExtBlk = 0;
                /*printf("extSect=%ld\n",extSect);*/
            }
            nSect = AdfBitm.adfGet1FreeBlock(file.volume);
            if (nSect == -1) {
                return -1;
            }

            /*printf("adfCreateNextFileBlock ext %ld\n",nSect);*/

            file.currentExt.dataBlocks[AdfConstants.MAX_DATABLK - 1 - file.posInExtBlk] = nSect;
            file.currentExt.highSeq++;
            file.posInExtBlk++;
        }

        /* builds OFS header */
        if (AdfConstants.isOFS(file.volume.dosType & 0xFF)) {
            /* writes previous data block and link it  */
            if (file.pos >= blockSize) {
                data.nextData = nSect;
                /* need to write back data into currentData buffer */
                ByteBuffer wb = ByteBuffer.allocate(512).order(ByteOrder.BIG_ENDIAN);
                data.write(wb, 0);
                for (int k = 0; k < 512; k++) {
                    file.currentData.put(k, wb.get(k));
                }
                adfWriteDataBlock(file.volume, file.curDataPtr, file.currentData);
                /*printf ("writedata=%d\n",file->curDataPtr);*/
            }
            /* initialize a new data block */
            BOFSDataBlock newData = new BOFSDataBlock();
            newData.seqNum = file.nDataBlock + 1;
            newData.dataSize = blockSize;
            newData.nextData = 0;
            newData.headerKey = file.fileHdr.headerKey;
            ByteBuffer nb = ByteBuffer.allocate(512).order(ByteOrder.BIG_ENDIAN);
            newData.write(nb, 0);
            for (int k = 0; k < 512; k++) {
                file.currentData.put(k, nb.get(k));
            }
        } else {
            if (file.pos >= blockSize) {
                adfWriteDataBlock(file.volume, file.curDataPtr, file.currentData);
                /*printf ("writedata=%d\n",file->curDataPtr);*/
                for (int k = 0; k < 512; k++) {
                    file.currentData.put(k, (byte) 0);
                }
            }
        }

        /*printf("datablk=%d\n",nSect);*/
        file.curDataPtr = nSect;
        file.nDataBlock++;

        return nSect;
    }

    /*
     * adfPos2DataBlock
     *
     */

    public static int adfPos2DataBlock(int pos, int blockSize,
            int[] posInExtBlk, int[] posInDataBlk, int[] curDataN) {
        int extBlock = 0;

        posInDataBlk[0] = pos % blockSize;
        curDataN[0] = pos / blockSize;
        if (posInDataBlk[0] == 0) {
            curDataN[0]++;
        }
        if (curDataN[0] < 72) {
            posInExtBlk[0] = 0;
            return -1;
        } else {
            posInExtBlk[0] = (pos - 72 * blockSize) % blockSize;
            extBlock = (pos - 72 * blockSize) / blockSize;
            if (posInExtBlk[0] == 0) {
                extBlock++;
            }
            return extBlock;
        }
    }

    /*
     * adfReadDataBlock
     *
     */

    public static AdfError adfReadDataBlock(Volume vol, int nSect, ByteBuffer data) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        BOFSDataBlock dBlock = null;
        AdfError rc = AdfError.RC_OK;

        AdfDisk.adfReadBlock(vol, nSect, buf);

        for (int i = 0; i < 512; i++) {
            data.put(i, buf[i]);
        }

        if (AdfConstants.isOFS(vol.dosType & 0xFF)) {
            ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
            dBlock = BOFSDataBlock.read(bb, 0);
            /*printf("adfReadDataBlock %ld\n",nSect);*/

            long sum = AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE);
            if (dBlock.checkSum != sum) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadDataBlock : invalid checksum");
                }
            }
            if (dBlock.type != AdfConstants.T_DATA) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadDataBlock : id T_DATA not found");
                }
            }
            if (dBlock.dataSize < 0 || dBlock.dataSize > 488) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadDataBlock : dataSize incorrect");
                }
            }
            if (!isSectNumValid(vol, dBlock.headerKey)) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadDataBlock : headerKey out of range");
                }
            }
            if (!isSectNumValid(vol, dBlock.nextData)) {
                if (adfEnv != null && adfEnv.wFct != null) {
                    adfEnv.wFct.call("adfReadDataBlock : nextData out of range");
                }
            }
        }

        return rc;
    }

    /*
     * adfWriteDataBlock
     *
     */

    public static AdfError adfWriteDataBlock(Volume vol, int nSect, ByteBuffer data) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;
        BOFSDataBlock dataB = null;
        AdfError rc = AdfError.RC_OK;

        newSum = 0L;
        if (AdfConstants.isOFS(vol.dosType & 0xFF)) {
            ByteBuffer bb = data.duplicate().order(ByteOrder.BIG_ENDIAN);
            dataB = BOFSDataBlock.read(bb, 0);
            dataB.type = AdfConstants.T_DATA;
            ByteBuffer wb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
            dataB.write(wb, 0);
            newSum = AdfRaw.adfNormalSum(buf, 20, 512);
            swLong(buf, 20, newSum);
            /*        *(int32_t*)(buf+20) = swapLong((uint8_t*)&newSum);*/
            AdfDisk.adfWriteBlock(vol, nSect, buf);
        } else {
            byte[] raw = new byte[512];
            for (int i = 0; i < 512; i++) {
                raw[i] = data.get(i);
            }
            AdfDisk.adfWriteBlock(vol, nSect, raw);
        }
        /*printf("adfWriteDataBlock %ld\n",nSect);*/

        return rc;
    }

    /*
     * adfReadFileExtBlock
     *
     */

    public static AdfError adfReadFileExtBlock(Volume vol, int nSect, BFileExtBlock fext) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        AdfError rc = AdfError.RC_OK;

        AdfDisk.adfReadBlock(vol, nSect, buf);
        /*printf("read fext=%d\n",nSect);*/
        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        BFileExtBlock tmp = BFileExtBlock.read(bb, 0);
        copyFileExtBlock(tmp, fext);
        if (fext.checkSum != AdfRaw.adfNormalSum(buf, 20, AdfConstants.LOGICAL_BLOCK_SIZE)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : invalid checksum");
            }
        }
        if (fext.type != AdfConstants.T_LIST) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : type T_LIST not found");
            }
        }
        if (fext.secType != AdfConstants.ST_FILE) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : stype  ST_FILE not found");
            }
        }
        if (fext.headerKey != nSect) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : headerKey!=nSect");
            }
        }
        if (fext.highSeq < 0 || fext.highSeq > AdfConstants.MAX_DATABLK) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : highSeq out of range");
            }
        }
        if (!isSectNumValid(vol, fext.parent)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : parent out of range");
            }
        }
        if (fext.extension != 0 && !isSectNumValid(vol, fext.extension)) {
            if (adfEnv != null && adfEnv.wFct != null) {
                adfEnv.wFct.call("adfReadFileExtBlock : extension out of range");
            }
        }

        return rc;
    }

    /*
     * adfWriteFileExtBlock
     *
     */

    public static AdfError adfWriteFileExtBlock(Volume vol, int nSect, BFileExtBlock fext) {
        byte[] buf = new byte[AdfConstants.LOGICAL_BLOCK_SIZE];
        long newSum = 0;
        AdfError rc = AdfError.RC_OK;

        fext.type = AdfConstants.T_LIST;
        fext.secType = AdfConstants.ST_FILE;
        fext.dataSize = 0;
        fext.firstData = 0;

        ByteBuffer bb = ByteBuffer.wrap(buf).order(ByteOrder.BIG_ENDIAN);
        fext.write(bb, 0);
        newSum = AdfRaw.adfNormalSum(buf, 20, 512);
        swLong(buf, 20, newSum);
        /*    *(int32_t*)(buf+20) = swapLong((uint8_t*)&newSum);*/

        AdfDisk.adfWriteBlock(vol, nSect, buf);

        return rc;
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

    private static boolean isSectNumValid(Volume vol, int nSect) {
        return 0 <= nSect && nSect <= (vol.lastBlock - vol.firstBlock);
    }

    private static void copyFileExtBlock(BFileExtBlock src, BFileExtBlock dst) {
        dst.type = src.type;
        dst.headerKey = src.headerKey;
        dst.highSeq = src.highSeq;
        dst.dataSize = src.dataSize;
        dst.firstData = src.firstData;
        dst.checkSum = src.checkSum;
        System.arraycopy(src.dataBlocks, 0, dst.dataBlocks, 0, src.dataBlocks.length);
        System.arraycopy(src.r, 0, dst.r, 0, src.r.length);
        dst.info = src.info;
        dst.nextSameHash = src.nextSameHash;
        dst.parent = src.parent;
        dst.extension = src.extension;
        dst.secType = src.secType;
    }

    private static void entryBlockToFileHeader(BEntryBlock entry, BFileHeaderBlock fhdr) {
        fhdr.type = entry.type;
        fhdr.headerKey = entry.headerKey;
        fhdr.highSeq = entry.r1[0];
        fhdr.dataSize = entry.r1[1];
        fhdr.firstData = entry.r1[2];
        fhdr.checkSum = entry.checkSum;
        System.arraycopy(entry.hashTable, 0, fhdr.dataBlocks, 0, AdfConstants.MAX_DATABLK);
        fhdr.r1 = entry.r2[0];
        fhdr.r2 = entry.r2[1];
        fhdr.access = entry.access;
        fhdr.byteSize = entry.byteSize & 0xFFFFFFFFL;
        fhdr.commLen = entry.commLen;
        System.arraycopy(entry.comment, 0, fhdr.comment, 0, Math.min(entry.comment.length, fhdr.comment.length));
        System.arraycopy(entry.r3, 0, fhdr.r3, 0, Math.min(entry.r3.length, fhdr.r3.length));
        fhdr.days = entry.days;
        fhdr.mins = entry.mins;
        fhdr.ticks = entry.ticks;
        fhdr.nameLen = entry.nameLen;
        System.arraycopy(entry.name, 0, fhdr.fileName, 0, Math.min(entry.name.length, fhdr.fileName.length));
        fhdr.r4 = entry.r4;
        fhdr.real = entry.realEntry;
        fhdr.nextLink = entry.nextLink;
        System.arraycopy(entry.r5, 0, fhdr.r5, 0, Math.min(entry.r5.length, fhdr.r5.length));
        fhdr.nextSameHash = entry.nextSameHash;
        fhdr.parent = entry.parent;
        fhdr.extension = entry.extension;
        fhdr.secType = entry.secType;
    }

    private static BEntryBlock fileHeaderToEntryBlock(BFileHeaderBlock fhdr) {
        BEntryBlock e = new BEntryBlock();
        e.type = fhdr.type;
        e.headerKey = fhdr.headerKey;
        e.r1[0] = fhdr.highSeq;
        e.r1[1] = fhdr.dataSize;
        e.r1[2] = fhdr.firstData;
        e.checkSum = fhdr.checkSum;
        System.arraycopy(fhdr.dataBlocks, 0, e.hashTable, 0, AdfConstants.MAX_DATABLK);
        e.r2[0] = fhdr.r1;
        e.r2[1] = fhdr.r2;
        e.access = fhdr.access;
        e.byteSize = (int) (fhdr.byteSize & 0xFFFFFFFFL);
        e.commLen = fhdr.commLen;
        System.arraycopy(fhdr.comment, 0, e.comment, 0, Math.min(fhdr.comment.length, e.comment.length));
        e.days = fhdr.days;
        e.mins = fhdr.mins;
        e.ticks = fhdr.ticks;
        e.nameLen = fhdr.nameLen;
        System.arraycopy(fhdr.fileName, 0, e.name, 0, Math.min(fhdr.fileName.length, e.name.length));
        e.r4 = fhdr.r4;
        e.realEntry = fhdr.real;
        e.nextLink = fhdr.nextLink;
        System.arraycopy(fhdr.r5, 0, e.r5, 0, Math.min(fhdr.r5.length, e.r5.length));
        e.nextSameHash = fhdr.nextSameHash;
        e.parent = fhdr.parent;
        e.extension = fhdr.extension;
        e.secType = fhdr.secType;
        return e;
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

    private static boolean adfIsLeap(int y) {
        return (y % 100 == 0) ? (y % 400 == 0) : (y % 4 == 0);
    }
}
