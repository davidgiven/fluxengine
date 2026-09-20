package com.cowlark.fluxengine.core;

import java.io.InputStream;
import java.util.Properties;

/* Provides the application version, supplied at build time via Bazel stamping.
 *
 * Bazel generates version.properties via scripts/version.bzl from
 * STABLE_VERSION (scripts/workspace_status.sh as YY.MM.N per-minute UTC
 * where N = (DD-1)*1440 + HH*60 + MM, fits MSI 255.255.65535 and macOS
 * CFBundleVersion 1-3 integers). This is the same version passed to jpackage
 * as --app-version, so Version.get() reports the installer version. The value
 * is also available as -Dfluxengine.version if set; otherwise the properties
 * file is read. When built without --stamp, falls back to 1.0.0-dev.
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
