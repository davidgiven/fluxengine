package com.cowlark.fluxengine.core;

import java.io.InputStream;
import java.util.Properties;

/* Provides the application version, supplied at build time via Bazel stamping.
 *
 * Bazel generates version.properties via scripts/version.bzl from
 * STABLE_VERSION (scripts/workspace_status.sh as YYYY.MMDD.HHMM per-minute UTC,
 * each component <65535 so valid for Windows MSI ProductVersion). The value is
 * also available as -Dfluxengine.version if set; otherwise the properties file
 * is read. When built without --stamp, falls back to 1.0.0-dev.
 */
public final class Version
{
    private Version()
    {
    }

    public static String get()
    {
        /* System property takes precedence (useful for testing / java_binary
         * override), but falls back to stamped resource. */
        String v = System.getProperty("fluxengine.version", "");
        if (v != null && !v.isEmpty() && !v.startsWith("$("))
            return v;

        String[] paths = {
            "/version.properties",
            "/com/cowlark/fluxengine/core/version.properties"
        };
        for (String path : paths)
        {
            try (InputStream is = Version.class.getResourceAsStream(path))
            {
                if (is != null)
                {
                    Properties p = new Properties();
                    p.load(is);
                    String pv = p.getProperty("fluxengine.version", "");
                    if (pv != null && !pv.isEmpty() && !pv.startsWith("$("))
                        return pv;
                }
            } catch (Exception e)
            {
                /* Ignore and fall back. */
            }
        }
        return "1.0.0-dev";
    }
}
