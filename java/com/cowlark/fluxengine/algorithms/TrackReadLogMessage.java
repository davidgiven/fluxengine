package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogRenderer;
import com.cowlark.fluxengine.data.Record;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import java.util.Collections;
import java.util.IdentityHashMap;
import java.util.List;
import java.util.Set;

/**
 * We've just read a track (we might reread it if there are errors), ported
 * from lib/algorithms/readerwriter.cc.
 */
public record TrackReadLogMessage(List<Track> tracks, List<Sector> sectors)
        implements LogMessage
{
    @Override
    public void render(LogRenderer r)
    {
        /* The C++ dedupes these by pointer, so use identity. */
        Set<Sector> rawSectors = Collections.newSetFromMap(new IdentityHashMap<>());
        Set<Record> rawRecords = Collections.newSetFromMap(new IdentityHashMap<>());
        for (Track track : tracks)
        {
            rawSectors.addAll(track.allSectors);
            rawRecords.addAll(track.records);
        }

        double clock = 0;
        for (Sector sector : rawSectors)
            clock += sector.clockNs;
        if (!rawSectors.isEmpty())
            clock /= rawSectors.size();

        r.comma().add(String.format("%d raw records, %d raw sectors",
                rawRecords.size(),
                rawSectors.size()));
        if (clock != 0)
            r.comma().add(String.format("%.2fus clock (%.0fkHz)",
                    clock / 1000.0,
                    1000000.0 / clock));

        r.newline().add("sectors:");

        for (Sector sector : rawSectors)
            r.add(String.format("%d.%d.%d%s",
                    sector.location.logicalCylinder(),
                    sector.location.logicalHead(),
                    sector.location.logicalSector(),
                    Sector.statusToChar(sector.status)));

        int size = 0;
        for (Sector sector : sectors)
            size += sector.data.size();

        r.newline().add(String.format("%d bytes decoded\n", size));
    }
}
