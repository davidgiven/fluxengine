/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 * adflib.h
 *
 *  $Id$
 *
 *  general include file
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
import java.util.List;

/**
 * Java facade of {@code adflib.h} — re-exports all public ADFLib API as
 * static methods delegating to the appropriate {@code Adf*} class.
 *
 * <p>Mirrors {@code adflib.h} grouping and naming verbatim:
 * <pre>
 *   util, dir, file, volume, device, dump device, env, link, salv,
 *   middle level API, low level API
 * </pre>
 * Keeps original C control flow delegates; return codes use {@link AdfError}.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment where applicable in the facade declarations.
 */
public final class Adflib {

    private Adflib() {
    }

    /* util */

    public static AdfList newCell(AdfList list, Object content) {
        return AdfUtil.newCell(list, content);
    }

    public static void freeList(AdfList list) {
        AdfUtil.freeList(list);
    }

    /* dir */

    public static Entry adfFindEntry(Volume vol, String name) {
        return AdfDir.adfFindEntry(vol, name);
    }

    public static AdfError adfToRootDir(Volume vol) {
        return AdfDir.adfToRootDir(vol);
    }

    public static AdfError adfCreateDir(Volume vol, int parent, String name) {
        return AdfDir.adfCreateDir(vol, parent, name);
    }

    public static AdfError adfChangeDir(Volume vol, String name) {
        return AdfDir.adfChangeDir(vol, name);
    }

    public static AdfError adfParentDir(Volume vol) {
        return AdfDir.adfParentDir(vol);
    }

    public static AdfError adfRemoveEntry(Volume vol, int pSect, String name) {
        return AdfDir.adfRemoveEntry(vol, pSect, name);
    }

    public static AdfList adfGetDirEnt(Volume vol, int nSect) {
        return AdfDir.adfGetDirEnt(vol, nSect);
    }

    public static AdfList adfGetRDirEnt(Volume vol, int nSect, boolean recurs) {
        return AdfDir.adfGetRDirEnt(vol, nSect, recurs);
    }

    public static void printEntry(Entry entry) {
        AdfDir.printEntry(entry);
    }

    public static void adfFreeDirList(AdfList list) {
        AdfDir.adfFreeDirList(list);
    }

    public static void adfFreeEntry(Entry entry) {
        AdfDir.adfFreeEntry(entry);
    }

    public static AdfError adfRenameEntry(Volume vol, int pSect, String oldName, int nPSect, String pNew) {
        return AdfDir.adfRenameEntry(vol, pSect, oldName, nPSect, pNew);
    }

    public static AdfError adfSetEntryAccess(Volume vol, int parSect, String name, int newAcc) {
        return AdfDir.adfSetEntryAccess(vol, parSect, name, newAcc);
    }

    public static AdfError adfSetEntryComment(Volume vol, int parSect, String name, String newCmt) {
        return AdfDir.adfSetEntryComment(vol, parSect, name, newCmt);
    }

    /* file */

    public static int adfFileRealSize(long size, int blockSize, int[] dataN, int[] extN) {
        return AdfFile.adfFileRealSize(size, blockSize, dataN, extN);
    }

    public static File adfOpenFile(Volume vol, String name, String mode) {
        return AdfFile.adfOpenFile(vol, name, mode);
    }

    public static void adfCloseFile(File file) {
        AdfFile.adfCloseFile(file);
    }

    public static int adfReadFile(File file, int n, byte[] buffer) {
        return AdfFile.adfReadFile(file, n, ByteBuffer.wrap(buffer).order(ByteOrder.BIG_ENDIAN));
    }

    public static int adfReadFile(File file, int n, ByteBuffer buffer) {
        return AdfFile.adfReadFile(file, n, buffer);
    }

    public static boolean adfEndOfFile(File file) {
        return AdfFile.adfEndOfFile(file);
    }

    public static int adfWriteFile(File file, int n, byte[] buffer) {
        return AdfFile.adfWriteFile(file, n, ByteBuffer.wrap(buffer).order(ByteOrder.BIG_ENDIAN));
    }

    public static int adfWriteFile(File file, int n, ByteBuffer buffer) {
        return AdfFile.adfWriteFile(file, n, buffer);
    }

    public static void adfFlushFile(File file) {
        AdfFile.adfFlushFile(file);
    }

    public static void adfFileSeek(File file, long pos) {
        AdfFile.adfFileSeek(file, pos);
    }

    /* volume */

    public static AdfError adfInstallBootBlock(Volume vol, byte[] code) {
        return AdfDisk.adfInstallBootBlock(vol, code);
    }

    public static Volume adfMount(Device dev, int nPart, boolean readOnly) {
        return AdfDisk.adfMount(dev, nPart, readOnly);
    }

    public static void adfUnMount(Volume vol) {
        AdfDisk.adfUnMount(vol);
    }

    public static void adfVolumeInfo(Volume vol) {
        AdfDisk.adfVolumeInfo(vol);
    }

    /* device */

    public static void adfDeviceInfo(Device dev) {
        AdfHd.adfDeviceInfo(dev);
    }

    public static Device adfMountDev(Device dev) {
        return AdfHd.adfMountDev(dev);
    }

    @Deprecated
    public static Device adfMountDev(String filename, boolean ro) {
        return AdfHd.adfMountDev(filename, ro);
    }

    public static void adfUnMountDev(Device dev) {
        AdfHd.adfUnMountDev(dev);
    }

    public static AdfError adfCreateHd(Device dev, int n, List<Partition> partList) {
        return AdfHd.adfCreateHd(dev, n, partList);
    }

    public static AdfError adfCreateFlop(Device dev, String volName, int volType) {
        return AdfHd.adfCreateFlop(dev, volName, volType);
    }

    @Deprecated
    public static AdfError adfCreateHdFile(Device dev, String volName, int volType) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    /* dump device */

    @Deprecated
    public static Device adfCreateDumpDevice(String filename, int cyl, int heads, int sec) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfInitDumpDevice(Device dev, String name, boolean ro) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfReadDumpSector(Device dev, int n, int size, byte[] buf) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfReadDumpSector(Device dev, int n, int size, ByteBuffer buf) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfWriteDumpSector(Device dev, int n, int size, byte[] buf) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfWriteDumpSector(Device dev, int n, int size, ByteBuffer buf) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    @Deprecated
    public static AdfError adfReleaseDumpDevice(Device dev) {
        throw new UnsupportedOperationException("dump device removed; use Device subclass");
    }

    /* env */

    public static void adfEnvInitDefault() {
        AdfEnv.adfEnvInitDefault();
    }

    public static void adfEnvCleanUp() {
        AdfEnv.adfEnvCleanUp();
    }

    public static void adfChgEnvProp(int prop, Object pNew) {
        AdfEnv.adfChgEnvProp(prop, pNew);
    }

    public static String adfGetVersionNumber() {
        return AdfEnv.adfGetVersionNumber();
    }

    public static String adfGetVersionDate() {
        return AdfEnv.adfGetVersionDate();
    }

    /* obsolete */

    public static void adfSetEnvFct(Env.StringCallback e, Env.StringCallback w, Env.StringCallback v) {
        AdfEnv.adfSetEnvFct(e, w, v);
    }

    public static void adfSetEnvFct(Env.StringCallback e, Env.StringCallback w, Env.StringCallback v,
            Env.NotifyCallback n) {
        AdfEnv.adfSetEnvFct(e, w, v, n);
    }

    /* link */

    public static AdfError adfBlockPtr2EntryName(Volume vol, int nSect, int lPar,
            String[] name, int[] size) {
        return AdfLink.adfBlockPtr2EntryName(vol, nSect, lPar, name, size);
    }

    /* salv */

    public static AdfList adfGetDelEnt(Volume vol) {
        return AdfSalv.adfGetDelEnt(vol);
    }

    public static AdfError adfUndelEntry(Volume vol, int parent, int nSect) {
        return AdfSalv.adfUndelEntry(vol, parent, nSect);
    }

    public static void adfFreeDelList(AdfList list) {
        AdfSalv.adfFreeDelList(list);
    }

    public static AdfError adfCheckEntry(Volume vol, int nSect, int level) {
        return AdfSalv.adfCheckEntry(vol, nSect, level);
    }

    public static AdfError adfReadGenBlock(Volume vol, int nSect, GenBlock block) {
        return AdfSalv.adfReadGenBlock(vol, nSect, block);
    }

    /* middle level API */

    public static boolean isSectNumValid(Volume vol, int nSect) {
        return AdfDisk.isSectNumValid(vol, nSect);
    }

    /* low level API */

    public static AdfError adfReadBlock(Volume vol, int nSect, byte[] buf) {
        return AdfDisk.adfReadBlock(vol, nSect, buf);
    }

    public static AdfError adfReadBlock(Volume vol, int nSect, ByteBuffer buf) {
        return AdfDisk.adfReadBlock(vol, nSect, buf);
    }

    public static AdfError adfWriteBlock(Volume vol, int nSect, byte[] buf) {
        return AdfDisk.adfWriteBlock(vol, nSect, buf);
    }

    public static AdfError adfWriteBlock(Volume vol, int nSect, ByteBuffer buf) {
        return AdfDisk.adfWriteBlock(vol, nSect, buf);
    }

    public static int adfCountFreeBlocks(Volume vol) {
        return AdfBitm.adfCountFreeBlocks(vol);
    }

    /* additional salv helpers exposed via adflib.h counterpart */

    public static void adfFreeGenBlock(GenBlock block) {
        AdfSalv.adfFreeGenBlock(block);
    }

    /* util helpers */

    public static void swLong(byte[] buf, int off, long val) {
        AdfUtil.swLong(buf, off, val);
    }

    public static void swLong(ByteBuffer buf, int off, long val) {
        AdfUtil.swLong(buf, off, val);
    }

    public static void swShort(byte[] buf, int off, int val) {
        AdfUtil.swShort(buf, off, val);
    }

    public static void swShort(ByteBuffer buf, int off, int val) {
        AdfUtil.swShort(buf, off, val);
    }

    public static void adfDays2Date(int days, int[] yy, int[] mm, int[] dd) {
        AdfUtil.adfDays2Date(days, yy, mm, dd);
    }

    public static boolean adfIsLeap(int y) {
        return AdfUtil.adfIsLeap(y);
    }

    public static void adfTime2AmigaTime(DateTime dt, int[] day, int[] min, int[] ticks) {
        AdfUtil.adfTime2AmigaTime(dt, day, min, ticks);
    }

    public static DateTime adfGiveCurrentTime() {
        return AdfUtil.adfGiveCurrentTime();
    }

    public static void dumpBlock(byte[] buf) {
        AdfUtil.dumpBlock(buf);
    }

    public static void dumpBlock(ByteBuffer buf) {
        AdfUtil.dumpBlock(buf);
    }
}
