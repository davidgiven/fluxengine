/*
 *  ADF Library. (C) 1997-2002 Laurent Clevy
 *
 *  adf_str.h — struct Env
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
 * {@code struct Env} — global callbacks / environment.
 *
 * <pre>
 * struct Env
 * {
 *     void (*vFct)(char*); // verbose callback function
 *     void (*wFct)(char*); // warning callback function
 *     void (*eFct)(char*); // error callback function
 *     void (*notifyFct)(SECTNUM, int);
 *     BOOL useNotify;
 *     void (*rwhAccess)(SECTNUM, SECTNUM, BOOL);
 *     BOOL useRWAccess;
 *     void (*progressBar)(int);
 *     BOOL useProgressBar;
 *     BOOL useDirCache;
 * };
 * </pre>
 * <p>
 * In Java, function pointers become functional interfaces.
 */
public final class Env
{

    @FunctionalInterface
    public interface StringCallback
    {
        void call(String msg);
    }

    @FunctionalInterface
    public interface NotifyCallback
    {
        void notify(int sectNum, int code);
    }

    @FunctionalInterface
    public interface RwhAccessCallback
    {
        void access(int sectNumStart, int sectNumEnd, boolean write);
    }

    @FunctionalInterface
    public interface ProgressCallback
    {
        void progress(int percent);
    }

    public StringCallback vFct; /* verbose callback function */
    public StringCallback wFct; /* warning callback function */
    public StringCallback eFct; /* error callback function */

    public NotifyCallback notifyFct;
    public boolean useNotify;

    public RwhAccessCallback rwhAccess;
    public boolean useRWAccess;

    public ProgressCallback progressBar;
    public boolean useProgressBar;

    public boolean useDirCache;

    public Env()
    {
    }
}
