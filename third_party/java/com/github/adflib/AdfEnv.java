/*
 *  ADF Library
 *
 *  adf_env.c / adf_env.h
 *
 *  $Id$
 *
 *  library context and customization code
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

/**
 * Java port of {@code adf_env.c} / {@code adf_env.h}.
 *
 * <p>Keeps original C control flow and helper naming. High-level objects use
 * normal Java classes ({@link Env}). Return codes use {@link AdfError}.
 *
 * <p>Original C comment blocks are preserved verbatim with a blank line before
 * each {@code /* } code comment.
 */
public final class AdfEnv {

    private AdfEnv() {
    }

    /** Global environment — mirrors {@code extern struct Env adfEnv}. */
    public static Env adfEnv = AdfRaw.adfEnv;

    private static void rwHeadAccess(int physical, int logical, boolean write) {
        /* display the physical sector, the logical block, and if the access is read or write */

        System.err.printf("phy %d / log %d : %c\n", physical, logical, write ? 'W' : 'R');
    }

    private static void progressBar(int perCentDone) {
        System.err.printf("%d %% done\n", perCentDone);
    }

    private static void Warning(String msg) {
        System.err.printf("Warning <%s>\n", msg);
    }

    private static void Error(String msg) {
        System.err.printf("Error <%s>\n", msg);
        /*    exit(1);*/
    }

    private static void Verbose(String msg) {
        System.err.printf("Verbose <%s>\n", msg);
    }

    private static void Changed(int nSect, int changedType) {
        /*    switch(changedType) {
        case ST_FILE:
            fprintf(stderr,"Notification : sector %ld (FILE)\n",nSect);
            break;
        case ST_DIR:
            fprintf(stderr,"Notification : sector %ld (DIR)\n",nSect);
            break;
        case ST_ROOT:
            fprintf(stderr,"Notification : sector %ld (ROOT)\n",nSect);
            break;
        default:
            fprintf(stderr,"Notification : sector %ld (???)\n",nSect);
        }
        */
    }

    /*
     * adfInitEnv
     *
     */

    public static void adfEnvInitDefault() {
        /*    char str[80];*/

        /* internal checking */

        /*    if (sizeof(short)!=2)
            { fprintf(stderr,"Compilation error : sizeof(short)!=2\n"); exit(1); }
            ... size checks for bEntryBlock etc. are not applicable in Java;
                buffer read/write classes enforce 512-byte sizes at runtime.
        */

        adfEnv.wFct = AdfEnv::Warning;
        adfEnv.eFct = AdfEnv::Error;
        adfEnv.vFct = AdfEnv::Verbose;
        adfEnv.notifyFct = AdfEnv::Changed;
        adfEnv.rwhAccess = AdfEnv::rwHeadAccess;
        adfEnv.progressBar = AdfEnv::progressBar;

        adfEnv.useDirCache = false;
        adfEnv.useRWAccess = false;
        adfEnv.useNotify = false;
        adfEnv.useProgressBar = false;

        /*    sprintf(str,"ADFlib %s (%s)",adfGetVersionNumber(),adfGetVersionDate());
            (*adfEnv.vFct)(str);
        */
        /* nativeFct removed — Device is now abstract */

        /* adfInitNativeFct() — no-op in Java port (native device not supported) */
    }

    /*
     * adfEnvCleanUp
     *
     */

    public static void adfEnvCleanUp() {
        /* nativeFct removed — Device is now abstract */
    }

    /*
     * adfChgEnvProp
     *
     */

    public static void adfChgEnvProp(int prop, Object newval) {
        switch (prop) {
        case AdfConstants.PR_VFCT:
            adfEnv.vFct = (Env.StringCallback) newval;
            break;
        case AdfConstants.PR_WFCT:
            adfEnv.wFct = (Env.StringCallback) newval;
            break;
        case AdfConstants.PR_EFCT:
            adfEnv.eFct = (Env.StringCallback) newval;
            break;
        case AdfConstants.PR_NOTFCT:
            adfEnv.notifyFct = (Env.NotifyCallback) newval;
            break;
        case AdfConstants.PR_USE_NOTFCT:
            adfEnv.useNotify = (Boolean) newval;
            break;
        case AdfConstants.PR_PROGBAR:
            adfEnv.progressBar = (Env.ProgressCallback) newval;
            break;
        case AdfConstants.PR_USE_PROGBAR:
            adfEnv.useProgressBar = (Boolean) newval;
            break;
        case AdfConstants.PR_USE_RWACCESS:
            adfEnv.useRWAccess = (Boolean) newval;
            break;
        case AdfConstants.PR_RWACCESS:
            adfEnv.rwhAccess = (Env.RwhAccessCallback) newval;
            break;
        case AdfConstants.PR_USEDIRC:
            adfEnv.useDirCache = (Boolean) newval;
            break;
        default:
            break;
        }
    }

    /*
     *  adfSetEnv
     *
     */

    public static void adfSetEnvFct(Env.StringCallback eFct, Env.StringCallback wFct,
            Env.StringCallback vFct, Env.NotifyCallback notFct) {
        if (eFct != null) {
            adfEnv.eFct = eFct;
        }
        if (wFct != null) {
            adfEnv.wFct = wFct;
        }
        if (vFct != null) {
            adfEnv.vFct = vFct;
        }
        if (notFct != null) {
            adfEnv.notifyFct = notFct;
        }
    }

    /** Obsolete 3-arg overload — mirrors {@code adflib.h} {@code adfSetEnvFct(e,w,v)}. */

    public static void adfSetEnvFct(Env.StringCallback eFct, Env.StringCallback wFct,
            Env.StringCallback vFct) {
        if (eFct != null) {
            adfEnv.eFct = eFct;
        }
        if (wFct != null) {
            adfEnv.wFct = wFct;
        }
        if (vFct != null) {
            adfEnv.vFct = vFct;
        }
    }

    /*
     * adfGetVersionNumber
     *
     */

    public static String adfGetVersionNumber() {
        return AdfConstants.ADFLIB_VERSION;
    }

    /*
     * adfGetVersionDate
     *
     */

    public static String adfGetVersionDate() {
        return AdfConstants.ADFLIB_DATE;
    }
}
