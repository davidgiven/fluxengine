package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.external.Catweasel;
import com.cowlark.fluxengine.data.Fluxmap;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Iterator over the flux revolutions of one track in a DMK stream directory,
 * ported from lib/fluxsource/dmkfluxsource.cc.
 */
class DmkFluxSourceIterator implements FluxSourceIterator
{
    private final String path;
    private final int track;
    private final int side;
    private int count = 0;

    DmkFluxSourceIterator(String path, int track, int side)
    {
        this.path = path;
        this.track = track;
        this.side = side;
    }

    private String getPath()
    {
        return String.format("%s/C_S%01dT%02d.%03d", path, side, track, count);
    }

    @Override
    public boolean hasNext()
    {
        return Files.exists(Path.of(getPath()));
    }

    @Override
    public Fluxmap next()
    {
        String path = getPath();
        Logger.logf("DMK: reading %s", path);
        Bytes bytes = readFile(path);
        count++;
        return Catweasel.decodeCatweaselData(bytes, 1e9 / 7080500.0);
    }

    private static Bytes readFile(String filename)
    {
        try
        {
            return new Bytes(Files.readAllBytes(Path.of(filename)));
        } catch (IOException e)
        {
            throw new FluxEngineException(
                    "cannot open input file '" + filename + "': " + e.getMessage());
        }
    }
}
