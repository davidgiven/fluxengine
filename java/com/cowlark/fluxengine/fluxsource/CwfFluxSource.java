package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.external.Catweasel;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Locations;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;
import java.util.Map;
import java.util.TreeMap;

/**
 * A flux source which reads a CWF flux file, ported from
 * lib/fluxsource/cwffluxsource.cc.
 */
public class CwfFluxSource extends TrivialFluxSource
{
    private final Bytes data;
    private final double clockPeriod;
    private final Map<CylinderHead, int[]> trackOffsets = new TreeMap<>();
    protected ConfigProto extraConfig;

    public CwfFluxSource(CwfFluxSourceProto config)
    {
        data = readFile(config.getFilename());

        ByteReader br = new ByteReader(data);
        Bytes fileId = br.read(4);
        if ((fileId.getByte(0) != 'C') || (fileId.getByte(1) != 'W') ||
                (fileId.getByte(2) != 'S') || (fileId.getByte(3) != 'F'))
            throw new FluxEngineException("input not a CWF file");

        br.read8(); /* creator */
        br.read8(); /* file_type */
        br.read8(); /* version */
        int clockRate = br.read8();
        br.read8(); /* drive_type */
        int cylinders = br.read8();
        int heads = br.read8();
        br.read8(); /* index_mark */
        int step = br.read8();
        br.skip(15); /* filler */
        br.skip(100); /* comment */

        switch (clockRate)
        {
            case 1:
                clockPeriod = 1e9 / 14161000.0;
                break;
            case 2:
                clockPeriod = 1e9 / 28322000.0;
                break;
            default:
                throw new FluxEngineException("unsupported clock rate");
        }

        Logger.logf("CWF %dx%d = %d cylinders, %d heads",
                cylinders,
                heads,
                cylinders * step,
                heads);
        Logger.logf("CWF sample clock rate: %d MHz", (int) (1e3 / clockPeriod));

        int numTracks = cylinders * heads;
        for (int i = 0; i < numTracks; i++)
        {
            int trackCylinder = br.read8();
            int trackHead = br.read8();
            br.skip(2); /* unused */
            int length = br.readLe32();
            int dataLength = length - 8;
            int pos = br.pos();
            trackOffsets.put(new CylinderHead(trackCylinder * step, trackHead),
                    new int[]{pos, dataLength});
            br.skip(dataLength);
        }

        List<CylinderHead> chs = new ArrayList<>(trackOffsets.keySet());
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(chs));
        extraConfig = builder.build();
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

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public Fluxmap readSingleFlux(FluxReadParameters parameters)
    {
        int[] offset = trackOffsets.get(new CylinderHead(parameters.cylinder(), parameters.head()));
        if (offset == null)
            return new Fluxmap();

        Bytes fluxdata = data.slice(offset[0], offset[1]);
        return Catweasel.decodeCatweaselData(fluxdata, clockPeriod);
    }
}
