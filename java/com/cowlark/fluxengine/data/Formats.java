package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.ConcurrentHashMap;
import org.protowire.pxf.Pxf;
import org.protowire.pxf.PxfException;

/**
 * The built-in format configurations, loaded on demand from the classpath
 * PXF resources in java/com/cowlark/fluxengine/arch.
 */
public final class Formats
{
    private static final String RESOURCE_DIR = "formats";
    private static final String RESOURCE_SUFFIX = ".pxf";
    private static final String NAMES_RESOURCE = RESOURCE_DIR + "/names.txt";

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
        /* Read the source configuration from the classpath. */
        String resource = "/" + RESOURCE_DIR + "/" + name + RESOURCE_SUFFIX;
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

        /* Include the resource name when reporting malformed built-in configurations. */
        try
        {
            return parse(data);
        } catch (FluxEngineException e)
        {
            throw new FluxEngineException("invalid format data in " + resource + ": " + e, e);
        }
    }

    /* Parses PXF, retaining textproto support for older configurations. */
    public static ConfigProto parse(byte[] data)
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        try
        {
            Pxf.unmarshal(data, builder);
            return builder.build();
        } catch (PxfException pxfException)
        {
            /* A failed decode may have modified the builder, so start afresh. */
            builder = ConfigProto.newBuilder();
            try
            {
                TextFormat.merge(new String(data, StandardCharsets.UTF_8), builder);
                return builder.build();
            } catch (TextFormat.ParseException textprotoException)
            {
                pxfException.addSuppressed(textprotoException);
                throw new FluxEngineException(
                        "invalid format data: PXF: " + pxfException
                                + "; textproto: " + textprotoException,
                        pxfException);
            }
        }
    }

    /* Returns the list of format names from the generated names index. */
    private static List<String> scanNames()
    {
        String contents;
        try (InputStream stream = Formats.class.getResourceAsStream("/" + NAMES_RESOURCE))
        {
            if (stream == null)
                throw new FluxEngineException("format resource not found: " + NAMES_RESOURCE);
            contents = new String(stream.readAllBytes(), StandardCharsets.UTF_8);
        } catch (IOException e)
        {
            throw new FluxEngineException(
                    "cannot read format resource " + NAMES_RESOURCE + ": " + e);
        }

        List<String> names = new ArrayList<>();
        for (String line : contents.split("\n"))
        {
            if (!line.isEmpty())
                names.add(line);
        }
        Collections.sort(names);
        return names;
    }
}
