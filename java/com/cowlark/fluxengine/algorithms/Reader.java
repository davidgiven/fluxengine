package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.LogMessage.BeginOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.BeginReadOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.EndOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.EndReadOperationLogMessage;
import com.cowlark.fluxengine.core.LogMessage.OperationProgressLogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.Utils;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Disk;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.PhysicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;

/**
 * Disk read/write algorithms, ported from lib/algorithms/readerwriter.cc.
 */
public final class Reader
{
    static enum ReadResult
    {
        GOOD_READ, BAD_AND_CAN_RETRY, BAD_AND_CAN_NOT_RETRY
    }

    static enum BadSectorsState
    {
        HAS_NO_BAD_SECTORS, HAS_BAD_SECTORS
    }

    private Reader()
    {
    }

    public static void readDiskCommand(ConfigProto config,
                                       DiskLayout diskLayout,
                                       FluxSource fluxSource,
                                       Decoder decoder,
                                       Disk disk)
    {
        FluxSinkFactory outputFluxSinkFactory = null;
        if (config.getDecoder().hasCopyFluxTo())
            outputFluxSinkFactory =
                    FluxSinkFactory.create(config, config.getDecoder().getCopyFluxTo());

        Map<CylinderHead, List<Track>> tracksByLogicalLocation = new HashMap<>();
        for (Map.Entry<CylinderHead, Track> entry : disk.tracksByPhysicalLocation.entries())
        {
            Track track = entry.getValue();
            tracksByLogicalLocation.computeIfAbsent(
                    new CylinderHead(track.ltl.logicalCylinder, track.ltl.logicalHead),
                    k -> new ArrayList<>()).add(track);
        }

        Logger.log(new BeginOperationLogMessage("Reading and decoding disk"));

        if (fluxSource.isHardware())
            disk.rotationalPeriod = Common.measureDiskRotation(config);
        else
            disk.rotationalPeriod = Common.getRotationalPeriodFromConfig(config);

        try (FluxSink outputFluxSink = outputFluxSinkFactory != null ?
                outputFluxSinkFactory.create() :
                null)
        {
            int index = 0;
            for (Map.Entry<CylinderHead, LogicalTrackLayout> entry :
                    diskLayout.layoutByLogicalLocation.entrySet())
            {
                CylinderHead logicalLocation = entry.getKey();
                LogicalTrackLayout ltl = entry.getValue();
                Logger.log(new OperationProgressLogMessage(
                        index * 100 / diskLayout.layoutByLogicalLocation.size()));
                index++;

                Common.testForEmergencyStop();

                List<Track> trackFluxes = tracksByLogicalLocation.computeIfAbsent(
                        logicalLocation,
                        k -> new ArrayList<>());
                List<Sector> trackSectors = new ArrayList<>();
                readAndDecodeTrack(
                        config,
                        diskLayout,
                        fluxSource,
                        decoder,
                        ltl,
                        trackFluxes,
                        trackSectors);

                /* Replace all tracks on the disk by the new combined set. */

                for (Track flux : trackFluxes)
                    disk.tracksByPhysicalLocation.removeAll(new CylinderHead(
                            flux.ptl.physicalCylinder,
                            flux.ptl.physicalHead));
                for (Track flux : trackFluxes)
                    disk.tracksByPhysicalLocation.put(
                            new CylinderHead(
                                    flux.ptl.physicalCylinder,
                                    flux.ptl.physicalHead),
                            flux);

                /* Likewise for sectors. */

                for (Sector sector : trackSectors)
                    disk.sectorsByPhysicalLocation.removeAll(sector.physicalLocation);
                for (Sector sector : trackSectors)
                    disk.sectorsByPhysicalLocation.put(sector.physicalLocation, sector);

                if (outputFluxSink != null)
                {
                    for (Track data : trackFluxes)
                        outputFluxSink.addFlux(
                                data.ptl.physicalCylinder,
                                data.ptl.physicalHead,
                                data.fluxmap);
                }

                if (config.getDecoder().getDumpRecords())
                {
                    List<com.cowlark.fluxengine.data.Record> sortedRecords = new ArrayList<>();
                    for (Track data : trackFluxes)
                        sortedRecords.addAll(data.records);
                    sortedRecords.sort(Comparator.comparingDouble(r -> r.startTimeNs));

                    System.out.println("\nRaw (undecoded) records follow:\n");
                    for (com.cowlark.fluxengine.data.Record record : sortedRecords)
                    {
                        System.out.printf(
                                "I+%.2fus with %.2fus clock%n",
                                record.startTimeNs / 1000.0,
                                record.clockNs / 1000.0);
                        Utils.hexdump(System.out, record.rawData);
                        System.out.println();
                    }
                }

                if (config.getDecoder().getDumpSectors())
                {
                    List<Sector> sectors = collectSectors(trackSectors, false);
                    sectors.sort(Comparator.comparing((Sector s) -> s.location.logicalCylinder())
                            .thenComparing((Sector s) -> s.location.logicalHead())
                            .thenComparing((Sector s) -> s.location.logicalSector()));

                    System.out.println("\nDecoded sectors follow:\n");
                    for (Sector sector : sectors)
                    {
                        System.out.printf(
                                "%d.%02d.%02d: I+%.2fus with %.2fus clock: " + "status %s%n",
                                sector.location.logicalCylinder(),
                                sector.location.logicalHead(),
                                sector.location.logicalSector(),
                                sector.headerStartTimeNs / 1000.0,
                                sector.clockNs / 1000.0,
                                Sector.statusToString(sector.status));
                        Utils.hexdump(System.out, sector.data);
                        System.out.println();
                    }
                }

                /* track can't be modified below this point. */

                List<Sector> allSectors = new ArrayList<>();
                for (Sector sector : disk.sectorsByPhysicalLocation.values())
                    allSectors.add(sector);
                allSectors = collectSectors(allSectors);
                disk.image = new Image(allSectors);
            }
        }

        if (disk.image == null)
            disk.image = new Image();

        Logger.log(new EndOperationLogMessage("Read complete"));
    }

    public static void readDiskCommand(ConfigProto config,
                                       DiskLayout diskLayout,
                                       FluxSource fluxSource,
                                       Decoder decoder,
                                       ImageWriter writer)
    {
        Disk disk = new Disk();
        readDiskCommand(config, diskLayout, fluxSource, decoder, disk);

        writer.printMap(disk.image);
        if (config.getDecoder().hasWriteCsvTo())
            writer.writeCsv(disk.image, config.getDecoder().getWriteCsvTo());
        writer.writeImage(disk.image);
    }

    /* Given a set of sectors, deduplicates them sensibly (e.g. if there is a
     * good and bad version of the same sector, the bad version is dropped). */
    static List<Sector> collectSectors(List<Sector> trackSectors, boolean collapseConflicts)
    {
        Map<LogicalLocation, List<Sector>> sectors = new LinkedHashMap<>();
        for (Sector sector : trackSectors)
            sectors.computeIfAbsent(sector.location, k -> new ArrayList<>()).add(sector);

        List<Sector> sectorSet = new ArrayList<>();
        for (Map.Entry<LogicalLocation, List<Sector>> entry : sectors.entrySet())
        {
            List<Sector> bucket = entry.getValue();
            Sector newSector = bucket.get(0);
            for (int i = 1; i < bucket.size(); i++)
            {
                Sector right = bucket.get(i);
                if ((newSector.status == Sector.Status.OK) && (right.status == Sector.Status.OK) &&
                        (!newSector.data.equals(right.data)))
                {
                    if (!collapseConflicts)
                    {
                        Sector s = copySector(right);
                        s.status = Sector.Status.CONFLICT;
                        sectorSet.add(s);
                    }
                    Sector s = copySector(newSector);
                    s.status = Sector.Status.CONFLICT;
                    newSector = s;
                    continue;
                }
                if (newSector.status == Sector.Status.CONFLICT)
                    continue;
                if (right.status == Sector.Status.CONFLICT)
                {
                    newSector = right;
                    continue;
                }
                if (newSector.status == Sector.Status.OK)
                    continue;
                if (right.status == Sector.Status.OK)
                    newSector = right;
            }
            sectorSet.add(newSector);
        }

        return sectorSet;
    }

    static List<Sector> collectSectors(List<Sector> trackSectors)
    {
        return collectSectors(trackSectors, true);
    }

    private static Sector copySector(Sector sector)
    {
        Sector s = new Sector(sector.location);
        s.status = sector.status;
        s.position = sector.position;
        s.clockNs = sector.clockNs;
        s.headerStartTimeNs = sector.headerStartTimeNs;
        s.headerEndTimeNs = sector.headerEndTimeNs;
        s.dataStartTimeNs = sector.dataStartTimeNs;
        s.dataEndTimeNs = sector.dataEndTimeNs;
        s.physicalLocation = sector.physicalLocation;
        s.data = sector.data;
        s.records = sector.records;
        return s;
    }

    static class CombinationResult
    {
        BadSectorsState result;
        List<Sector> sectors;
    }

    static CombinationResult combineRecordAndSectors(List<Track> tracks,
                                                     Decoder decoder,
                                                     LogicalTrackLayout ltl)
    {
        CombinationResult cr = new CombinationResult();
        cr.result = BadSectorsState.HAS_NO_BAD_SECTORS;
        List<Sector> trackSectors = new ArrayList<>();

        /* Add the sectors which were there. */

        for (Track track : tracks)
            trackSectors.addAll(track.allSectors);

        /* Add the sectors which should be there. */

        for (int sectorId : ltl.diskSectorOrder)
        {
            Sector sector =
                    new Sector(new LogicalLocation(ltl.logicalCylinder, ltl.logicalHead, sectorId));

            sector.status = Sector.Status.MISSING;
            sector.physicalLocation = new CylinderHead(ltl.physicalCylinder, ltl.physicalHead);
            trackSectors.add(sector);
        }

        /* Deduplicate. */

        cr.sectors = collectSectors(trackSectors);
        if (cr.sectors.isEmpty())
            cr.result = BadSectorsState.HAS_BAD_SECTORS;
        for (Sector sector : cr.sectors)
            if (sector.status != Sector.Status.OK)
                cr.result = BadSectorsState.HAS_BAD_SECTORS;

        return cr;
    }

    static class ReadGroupResult
    {
        ReadResult result;
        List<Sector> combinedSectors;
    }

    static ReadGroupResult readGroup(DiskLayout diskLayout,
                                     Common.FluxSourceIteratorHolder fluxSourceIteratorHolder,
                                     LogicalTrackLayout ltl,
                                     List<Track> tracks,
                                     Decoder decoder,
                                     ConfigProto config)
    {
        ReadGroupResult rgr = new ReadGroupResult();
        rgr.result = ReadResult.BAD_AND_CAN_NOT_RETRY;

        /* Before doing the read, look to see if we already have the necessary
         * sectors. */

        {
            CombinationResult cr = combineRecordAndSectors(tracks, decoder, ltl);
            rgr.combinedSectors = cr.sectors;
            if (cr.result == BadSectorsState.HAS_NO_BAD_SECTORS)
            {
                /* We have all necessary sectors, so can stop here. */
                rgr.result = ReadResult.GOOD_READ;
                if (config.getDecoder().getSkipUnnecessaryTracks())
                    return rgr;
            }
        }

        for (int offset = 0; offset < ltl.groupSize; offset += diskLayout.headWidth)
        {
            int physicalCylinder = ltl.physicalCylinder + offset;
            int physicalHead = ltl.physicalHead;
            PhysicalTrackLayout ptl = diskLayout.layoutByPhysicalLocation.get(new CylinderHead(
                    physicalCylinder,
                    physicalHead));

            /* Do the physical read. */

            Logger.log(new BeginReadOperationLogMessage(physicalCylinder, physicalHead));

            FluxSourceIterator fluxSourceIterator =
                    fluxSourceIteratorHolder.getIterator(physicalCylinder, physicalHead);
            if (!fluxSourceIterator.hasNext())
                continue;

            Fluxmap fluxmap = fluxSourceIterator.next();
            Logger.log(new EndReadOperationLogMessage());
            Logger.logf("%d ms in %d bytes", (int) (fluxmap.duration() / 1e6), fluxmap.bytes());

            Track flux = decoder.decodeToSectors(fluxmap, ptl);
            flux.normalisedSectors = collectSectors(flux.allSectors);
            tracks.add(flux);

            /* Decode what we've got so far. */

            CombinationResult cr = combineRecordAndSectors(tracks, decoder, ltl);
            rgr.combinedSectors = cr.sectors;
            if (cr.result == BadSectorsState.HAS_NO_BAD_SECTORS)
            {
                /* We have all necessary sectors, so can stop here. */
                rgr.result = ReadResult.GOOD_READ;
                if (config.getDecoder().getSkipUnnecessaryTracks())
                    break;
            } else if (fluxSourceIterator.hasNext())
            {
                /* The flux source claims it can do more reads, so mark this
                 * group as being retryable. */
                rgr.result = ReadResult.BAD_AND_CAN_RETRY;
            }
        }

        return rgr;
    }

    private static void readAndDecodeTrack(ConfigProto config,
                                           DiskLayout diskLayout,
                                           FluxSource fluxSource,
                                           Decoder decoder,
                                           LogicalTrackLayout ltl,
                                           List<Track> tracks,
                                           List<Sector> combinedSectors)
    {
        if (fluxSource.isHardware())
            Common.measureDiskRotation(config);

        Common.FluxSourceIteratorHolder fluxSourceIteratorHolder =
                new Common.FluxSourceIteratorHolder(fluxSource);
        int retriesRemaining = config.getDecoder().getRetries();
        for (; ; )
        {
            ReadGroupResult rgr =
                    readGroup(diskLayout, fluxSourceIteratorHolder, ltl, tracks, decoder, config);
            combinedSectors.clear();
            combinedSectors.addAll(rgr.combinedSectors);
            if (rgr.result == ReadResult.GOOD_READ)
                break;
            if (rgr.result == ReadResult.BAD_AND_CAN_NOT_RETRY)
            {
                Logger.logf("no more data; giving up");
                break;
            }

            if (retriesRemaining == 0)
            {
                Logger.logf("giving up");
                break;
            }

            if (fluxSource.isHardware())
            {
                Common.adjustTrackOnError(fluxSource, ltl.physicalCylinder, config);
                Logger.logf("retrying; %d retries remaining", retriesRemaining);
                retriesRemaining--;
            }
        }
    }

}
