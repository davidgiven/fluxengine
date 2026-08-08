package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.InvalidProtocolBufferException;
import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.nio.file.DirectoryStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.Collections;
import java.util.Enumeration;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import java.util.jar.JarEntry;
import java.util.jar.JarFile;

/**
 * The built-in format configurations, loaded on demand from the classpath
 * resources generated from the textpb files in src/formats, ported from the
 * C++ `formats` map in lib/config.
 */
public final class Formats
{
    private static final String RESOURCE_DIR = "formats";
    private static final String RESOURCE_PREFIX = "com/cowlark/fluxengine/data/" + RESOURCE_DIR + "/";
    private static final String RESOURCE_SUFFIX = ".bin";

    private static final ConcurrentHashMap<String, ConfigProto> cache = new ConcurrentHashMap<>();

    private Formats()
    {
    }

    /* Returns the config with the given name, loading it on demand, or null
     * if it doesn't exist. */
    public static ConfigProto get(String name)
    {
        ConfigProto config = cache.get(name);
        if (config == null)
        {
            config = load(name);
            if (config != null)
                cache.putIfAbsent(name, config);
        }
        return config;
    }

    /* Returns the names of all the available configs. */
    public static ImmutableList<String> all()
    {
        return ImmutableList.copyOf(scanNames());
    }

    private static ConfigProto load(String name)
    {
        String resource = RESOURCE_DIR + "/" + name + RESOURCE_SUFFIX;
        byte[] data;
        try (InputStream stream = Formats.class.getResourceAsStream(resource))
        {
            if (stream == null)
                return null;
            data = stream.readAllBytes();
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot read format resource " + resource + ": " + e);
        }

        try
        {
            return ConfigProto.parseFrom(data);
        } catch (InvalidProtocolBufferException e)
        {
            throw new FluxEngineException("invalid format data in " + resource + ": " + e);
        }
    }

    /* Scans the resources directory for the format files, returning their
     * names in sorted order. */
    private static List<String> scanNames()
    {
        URL location = Formats.class.getProtectionDomain().getCodeSource().getLocation();
        if (location == null)
            throw new FluxEngineException("cannot determine the location of the format resources");

        List<String> names = new ArrayList<>();
        try
        {
            if (location.getProtocol().equals("file"))
            {
                Path path = Path.of(location.toURI());
                if (Files.isDirectory(path))
                    scanDirectory(path.resolve(RESOURCE_PREFIX), names);
                else
                    scanJar(path, names);
            } else
            {
                throw new FluxEngineException(
                        "unsupported resource protocol: " + location.getProtocol());
            }
        } catch (Exception e)
        {
            throw new FluxEngineException("cannot scan format resources: " + e);
        }

        Collections.sort(names);
        return names;
    }

    private static void scanDirectory(Path dir, List<String> names) throws IOException
    {
        try (DirectoryStream<Path> stream = Files.newDirectoryStream(dir, "*" + RESOURCE_SUFFIX))
        {
            for (Path entry : stream)
            {
                String filename = entry.getFileName().toString();
                names.add(filename.substring(0, filename.length() - RESOURCE_SUFFIX.length()));
            }
        }
    }

    private static void scanJar(Path jar, List<String> names) throws IOException
    {
        try (JarFile jarFile = new JarFile(jar.toFile()))
        {
            Enumeration<JarEntry> entries = jarFile.entries();
            while (entries.hasMoreElements())
            {
                String name = entries.nextElement().getName();
                if (name.startsWith(RESOURCE_PREFIX) && name.endsWith(RESOURCE_SUFFIX))
                    names.add(name.substring(
                            RESOURCE_PREFIX.length(), name.length() - RESOURCE_SUFFIX.length()));
            }
        }
    }
}