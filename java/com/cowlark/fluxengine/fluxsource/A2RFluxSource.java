package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.core.FluxEngineException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.Arrays;

/* Selects the A2R reader using the file's four-byte magic number. */
public final class A2RFluxSource
{
    private A2RFluxSource()
    {
    }

    public static FluxSource create(A2rFluxSourceProto config)
    {
        byte[] magic;
        try (InputStream stream = Files.newInputStream(Path.of(config.getFilename())))
        {
            magic = stream.readNBytes(4);
        } catch (IOException e)
        {
            throw new FluxEngineException(
                    "cannot open input file '" + config.getFilename() + "': " + e.getMessage());
        }

        if (Arrays.equals(magic, new byte[] {'A', '2', 'R', '2'}))
            return new A2R2FluxSource(config);

        if (Arrays.equals(magic, new byte[] {'A', '2', 'R', '3'}))
            return new A2R3FluxSource(config);

        throw new FluxEngineException("unsupported A2R version");
    }
}
