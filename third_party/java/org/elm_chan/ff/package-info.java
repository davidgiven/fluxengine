/**
 * Java port of Elm Chan FatFs R0.16 (rev 80386).
 *
 * <p>Sources: {@code /home/dg/nonshared/fatfs/source/ff.c},
 * {@code ff.h}, {@code diskio.h} (diskio.c is sample code and is not ported);
 * {@code ffsystem.c} and {@code ffunicode.c} compile to nothing under the
 * baked-in configuration and are therefore represented by this package
 * documentation only.
 *
 * <p>Baked-in {@code ffconf.h} settings: {@code FF_FS_READONLY=0},
 * {@code FF_FS_MINIMIZE=0}, {@code FF_USE_FIND=0}, {@code FF_USE_MKFS=1 (enabled on request; dep/ffconf.h has 0)},
 * {@code FF_USE_FASTSEEK=0}, {@code FF_USE_EXPAND=0}, {@code FF_USE_CHMOD=0},
 * {@code FF_USE_LABEL=0}, {@code FF_USE_FORWARD=0}, {@code FF_USE_STRFUNC=0},
 * {@code FF_CODE_PAGE=932}, {@code FF_USE_LFN=0}, {@code FF_FS_RPATH=0},
 * {@code FF_VOLUMES=1}, {@code FF_MIN_SS=FF_MAX_SS=512}, {@code FF_LBA64=0},
 * {@code FF_USE_TRIM=0}, {@code FF_FS_TINY=0}, {@code FF_FS_EXFAT=0},
 * {@code FF_FS_NORTC=0}, {@code FF_FS_LOCK=0}, {@code FF_FS_REENTRANT=0}.
 *
 * <p>Deviations from the C original:
 * <ul>
 *   <li>{@code BYTE* dir} pointers into {@code win[]} are represented as int offsets.</li>
 *   <li>Disk I/O is injected per-volume via {@link org.elm_chan.ff.DiskIo} (no {@code pdrv} table).</li>
 *   <li>{@code get_fattime()} is supplied as a {@code LongSupplier} to {@link org.elm_chan.ff.FatFs}.</li>
 *   <li>Out-parameters use {@link org.elm_chan.ff.IntRef}/{@link org.elm_chan.ff.LongRef}.</li>
 *   <li>{@code f_mkfs(path, opt, work, len)} is exposed as {@code mkfs(path, opt)} — scratch buffer allocated internally (1 sector); work/len omitted.</li>
 * </ul>
 *
 * <p>Original FatFs license header is retained verbatim on each translated file.
 *
 * <p>ffsystem.c - A Sample Code of User Provided OS Dependent Functions for FatFs
 * header preserved by reference:
 * <pre>
 * /*------------------------------------------------------------------------* /
 * /* A Sample Code of User Provided OS Dependent Functions for FatFs        * /
 * /*------------------------------------------------------------------------* /
 * </pre>
 * Under the baked-in configuration (FF_USE_LFN != 3 and FF_FS_REENTRANT == 0)
 * the entire file is blanked (all functions are inside disabled #if blocks),
 * so no Java translation is required. The per-function headers that would have
 * been present are:
 * <ul>
 *   <li>/* Allocate/Free a Memory Block - ff_memalloc / ff_memfree * /</li>
 *   <li>/* Create a Mutex - ff_mutex_create * /</li>
 *   <li>/* Delete a Mutex - ff_mutex_delete * /</li>
 *   <li>/* Request a Grant to Access the Volume - ff_mutex_take * /</li>
 *   <li>/* Release a Grant to Access the Volume - ff_mutex_give * /</li>
 * </ul>
 *
 * <p>ffunicode.c - Unicode Handling Functions for FatFs
 * header preserved by reference:
 * <pre>
 * /*------------------------------------------------------------------------* /
 * /* Unicode Handling Functions for FatFs R0.13 and Later                   * /
 * /*------------------------------------------------------------------------* /
 * /* This module will occupy a huge memory in the .rodata section when the  * /
 * /* FatFs is configured for LFN with DBCS. If the system has a Unicode     * /
 * /* library for the code conversion, this module should be modified to use * /
 * /* it to avoid silly memory consumption.                                  * /
 * /*------------------------------------------------------------------------* /
 * </pre>
 * Under the baked-in configuration (FF_USE_LFN == 0) the entire file is blanked
 * ({@code #if FF_USE_LFN} ... {@code #endif}); all code conversion tables
 * (uni2oem932 etc.) and functions ff_oem2uni / ff_uni2oem / ff_wtoupper are
 * omitted.
 */
package org.elm_chan.ff;
