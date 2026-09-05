/*
 * libhfs - library for reading and writing Macintosh HFS volumes
 * Copyright (C) 1996-1998 Robert Leslie
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id: apple.h,v 1.1 1998/04/11 08:27:11 rob Exp $
 */

package org.mars.hfsutils;

public class BootBlkHdr {

    /* boot blocks signature */
    public short bbID;

    /* entry point to boot code */
    public long bbEntry;

    /* boot blocks version number */
    public short bbVersion;

    /* used internally */
    public short bbPageFlags;

    /* System filename */
    public char[] bbSysName = new char[16];

    /* Finder filename */
    public char[] bbShellName = new char[16];

    /* debugger filename */
    public char[] bbDbg1Name = new char[16];

    /* debugger filename */
    public char[] bbDbg2Name = new char[16];

    /* name of startup screen */
    public char[] bbScreenName = new char[16];

    /* name of startup program */
    public char[] bbHelloName = new char[16];

    /* name of system scrap file */
    public char[] bbScrapName = new char[16];

    /* number of FCBs to allocate */
    public short bbCntFCBs;

    /* number of event queue elements */
    public short bbCntEvts;

    /* system heap size on 128K Mac */
    public long bb128KSHeap;

    /* used internally */
    public long bb256KSHeap;

    /* system heap size on all machines */
    public long bbSysHeapSize;

    /* reserved */
    public short filler;

    /* additional system heap space */
    public long bbSysHeapExtra;

    /* fraction of RAM for system heap */
    public long bbSysHeapFract;
}
