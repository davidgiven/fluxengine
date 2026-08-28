/**
 * Java port of libhfs 3.2.6 (GPL).
 *
 * <p>Sources: {@code /home/dg/nonshared/fluxengine/dep/hfsutils/libhfs/*.[ch]}.
 *
 * <p>Baked-in configuration:
 * {@code DEBUG} is off ({@code ASSERT} is no-op),
 * {@code HAVE_MKTIME} is on (timezone via {@code java.time}).
 *
 * <p>Deviations from the C original:
 * <ul>
 *   <li>{@code memcmp.c} not ported (Java {@code Arrays.equals}).</li>
 *   <li>{@code config.h} not ported; baked-in settings documented here.</li>
 *   <li>OS layer is an abstract {@link org.mars.hfsutils.os.HfsOs} class injected
 *       into {@link Hfs#hfsMount}/{@link Hfs#hfsFormat} (replaces global {@code os_*}
 *       functions with {@code void*} priv).</li>
 *   <li>Error handling: internal functions return {@code -1/0/1} and set
 *       {@link Hfs#hfsError}/{@link Hfs#hfsErrno}; public API throws
 *       {@link HfsException} (an {@code IOException}).</li>
 *   <li>Out-parameters use single-element arrays ({@code long[1]}, {@code int[1]}).</li>
 * </ul>
 *
 * <p>Original libhfs license header is retained verbatim on each translated file.
 */
package org.mars.hfsutils;
