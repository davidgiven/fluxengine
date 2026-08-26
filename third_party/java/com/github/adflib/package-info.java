/**
 * Java port of ADFLib core headers 0.7.11a (January 20th, 2007).
 *
 * <p>Sources: {@code dep/adflib/src/adf_defs.h}, {@code adf_blk.h},
 * {@code adf_err.h}, {@code hd_blk.h}, {@code adf_str.h},
 * {@code defendian.h}, {@code prefix.h}.
 *
 * <p>Baked-in assumptions mirror the FatFs port style:
 * <ul>
 *   <li>ByteBuffer is used idiomatically for on-disk blocks — absolute
 *       {@code get(int)}/{@code put(int,byte)} without touching
 *       {@code position}/{@code limit}; {@code ByteOrder.BIG_ENDIAN}
 *       for ADF (Amiga/M68k) structures.</li>
 *   <li>High-level objects ({@link com.github.adflib.Device},
 *       {@link com.github.adflib.Volume}, {@link com.github.adflib.File},
 *       {@link com.github.adflib.Entry}) are plain POJOs.</li>
 *   <li>Return codes keep the C {@code RETCODE} numeric values via
 *       {@link com.github.adflib.AdfError} (overlapping values preserved).</li>
 *   <li>Out-parameters use {@code int[1]}/{@code long[1]}/{@code String[1]}</li>
 *   <li>Original {@code /* *&#47;} comments are preserved with a blank line before code comments;
 *       public API uses Javadoc {@code /**}.</li>
 *   <li>Helper names match the C originals where possible ({@code Short},
 *       {@code Long}, {@code swapShort}, etc. via {@link com.github.adflib.AdfEndian}).</li>
 * </ul>
 *
 * <p>Headers {@code defendian.h} and {@code prefix.h}:
 * {@code defendian.h} selected {@code LITT_ENDIAN} on little-endian hosts;
 * in Java this is represented by {@link com.github.adflib.AdfEndian} providing
 * both BE and LE helpers explicitly. {@code prefix.h} only defined
 * {@code PREFIX} ({@code __declspec(dllexport)} on WIN32DLL, empty elsewhere);
 * in Java symbol export is handled by the module system — no translation needed
 * beyond this note.
 *
 * <p>Original ADFLib license header is retained verbatim on each translated file.
 */
package com.github.adflib;
