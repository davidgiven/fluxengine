package com.cowlark.fluxengine.fluxsource;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.Locations;
import com.cowlark.fluxengine.data.Track;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.TreeSet;

/**
 * A flux source which reads from a disk held in memory, ported from
 * lib/fluxsource/memoryfluxsource.cc.
 */
public class MemoryFluxSource extends FluxSource
{
    private final Disk flux;
    protected ConfigProto extraConfig;

    public MemoryFluxSource(Disk flux)
    {
        this.flux = flux;

        TreeSet<CylinderHead> chs = new TreeSet<>();
        chs.addAll(flux.tracksByPhysicalLocation.keySet());

        List<CylinderHead> sortedChs = new ArrayList<>(chs);
        ConfigProto.Builder builder = ConfigProto.newBuilder();
        builder.getDriveBuilder().setTracks(Locations.convertCylinderHeadsToString(sortedChs));
        extraConfig = builder.build();
    }

    @Override
    public void adjustConfig(ConfigBuilder configBuilder)
    {
        configBuilder.mergeConfig(extraConfig);
    }

    @Override
    public FluxSourceIterator readFlux(FluxReadParameters parameters)
    {
        Collection<Track> tracks = flux.tracksByPhysicalLocation
                .asMap()
                .get(new CylinderHead(parameters.cylinder(), parameters.head()));
        if ((tracks != null) && !tracks.isEmpty())
            return new MemoryFluxSourceIterator(tracks);

        return new EmptyFluxSourceIterator();
    }
}
