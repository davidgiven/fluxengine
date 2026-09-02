package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.arch.Arch;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.SupplierOfAutocloseable;
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
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.imagereader.ImageReader;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;
import com.google.common.collect.ImmutableCollection;
import com.google.common.collect.ImmutableList;
import java.util.ArrayList;
import java.util.Comparator;
import java.util.HashMap;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Function;
import java.util.function.Predicate;

public abstract class ReadWriteFluxOperation extends FluxOperation<ReadWriteFluxOperation>
{
    private double diskRotationalPeriodNs;
    private Supplier<DiskLayout> diskLayoutSupplier;
    private SupplierOfAutocloseable<FluxSource> fluxSourceSupplier;
    private SupplierOfAutocloseable<FluxSinkFactory> fluxSinkFactorySupplier;
    private SupplierOfAutocloseable<UsbFactory> usbFactorySupplier;
    private Supplier<Decoder> decoderSupplier;
    private Supplier<Encoder> encoderSupplier;
    private SupplierOfAutocloseable<ImageReader> imageReaderSupplier;
    private SupplierOfAutocloseable<ImageWriter> imageWriterSupplier;

    static CombinationResult combineRecordAndSectors(List<Track> tracks, LogicalTrackLayout ltl)
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
                        Sector s = new Sector(right);
                        s.status = Sector.Status.CONFLICT;
                        sectorSet.add(s);
                    }
                    Sector s = new Sector(newSector);
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

    @Override
    public void init()
    {
        ConfigProto configProto = getConfig();

        diskLayoutSupplier = Suppliers.memoize(() -> new DiskLayout(configProto));
        usbFactorySupplier = new SupplierOfAutocloseable(() -> new UsbFactory(configProto));
        fluxSourceSupplier = new SupplierOfAutocloseable(() -> FluxSource.create(
                configProto,
                () -> usbFactorySupplier.get()));
        fluxSinkFactorySupplier = new SupplierOfAutocloseable(() -> FluxSinkFactory.create(
                configProto,
                () -> usbFactorySupplier.get()));
        decoderSupplier = Suppliers.memoize(() -> Arch.createDecoder(configProto));
        encoderSupplier = Suppliers.memoize(() -> Arch.createEncoder(
                configProto,
                getDiskRotationalPeriodNs()));
        imageWriterSupplier = new SupplierOfAutocloseable(() -> ImageWriter.create(configProto));
        imageReaderSupplier = new SupplierOfAutocloseable(() -> ImageReader.create(configProto));
    }

    public DiskLayout getDiskLayout()
    {
        return diskLayoutSupplier.get();
    }

    public FluxSource getFluxSource()
    {
        return fluxSourceSupplier.get();
    }

    public FluxSinkFactory getFluxSinkFactory()
    {
        return fluxSinkFactorySupplier.get();
    }

    public Decoder getDecoder()
    {
        return decoderSupplier.get();
    }

    public Encoder getEncoder()
    {
        return encoderSupplier.get();
    }

    public ImageReader getImageReader()
    {
        return imageReaderSupplier.get();
    }

    public ImageWriter getImageWriter()
    {
        return imageWriterSupplier.get();
    }

    public double getDiskRotationalPeriodNs()
    {
        if (diskRotationalPeriodNs != 0)
            return diskRotationalPeriodNs;
        diskRotationalPeriodNs = configProto.getDrive().getRotationalPeriodMs() * 1e6;
        if (diskRotationalPeriodNs == 0)
        {
            usbFactorySupplier.get().perform(device -> {
                Logger.log(new BeginOperationLogMessage("Measuring drive rotational speed"));
                Logger.log(new BeginSpeedOperationLogMessage());

                int retries = 5;
                do
                {
                    diskRotationalPeriodNs = device.getRotationalPeriod();
                    retries--;
                } while ((diskRotationalPeriodNs == 0) && (retries > 0));
                Logger.log(new EndOperationLogMessage(""));
            });
        }

        if (diskRotationalPeriodNs == 0)
            throw new FluxEngineException("Failed\nIs a disk in the drive?");

        Logger.log(new EndSpeedOperationLogMessage(diskRotationalPeriodNs));
        return diskRotationalPeriodNs;
    }

    @Override
    protected void onDispose()
    {
        closeResource(fluxSourceSupplier);
        closeResource(fluxSinkFactorySupplier);
        closeResource(usbFactorySupplier);
        closeResource(imageWriterSupplier);
        closeResource(imageReaderSupplier);
    }

    private void closeResource(SupplierOfAutocloseable<?> resource)
    {
        if (resource != null)
        {
            try
            {
                resource.close();
            } catch (Exception e)
            {
                throw new RuntimeException(e);
            }
        }
    }

    void adjustTrackOnError(int baseTrack)
    {
        switch (getConfig().getDrive().getErrorBehaviour())
        {
            case NOTHING:
                break;

            case RECALIBRATE:
                usbFactorySupplier.get().perform(device -> device.seek(0));
                break;

            case JIGGLE:
                usbFactorySupplier
                        .get()
                        .perform(device -> device.seek((baseTrack > 0) ?
                                (baseTrack - 1) :
                                (baseTrack + 1)));
                break;
        }
    }

    protected ReadGroupResult readGroup(
            Common.FluxSourceIteratorHolder fluxSourceIteratorHolder,
            LogicalTrackLayout ltl,
            List<Track> tracks)
    {
        ReadGroupResult rgr = new ReadGroupResult();
        rgr.result = ReadResult.BAD_AND_CAN_NOT_RETRY;

        /* Before doing the read, look to see if we already have the necessary
         * sectors. */

        {
            CombinationResult cr = combineRecordAndSectors(tracks, ltl);
            rgr.combinedSectors = cr.sectors;
            if (cr.result == BadSectorsState.HAS_NO_BAD_SECTORS)
            {
                /* We have all necessary sectors, so can stop here. */
                rgr.result = ReadResult.GOOD_READ;
                if (getConfig().getDecoder().getSkipUnnecessaryTracks())
                    return rgr;
            }
        }

        for (int offset = 0; offset < ltl.groupSize; offset += getDiskLayout().headWidth)
        {
            int physicalCylinder = ltl.physicalCylinder + offset;
            int physicalHead = ltl.physicalHead;
            PhysicalTrackLayout ptl = getDiskLayout().layoutByPhysicalLocation.get(new CylinderHead(
                    physicalCylinder,
                    physicalHead));

            /* Do the physical read. */

            Logger.log(new BeginReadOperationLogMessage(physicalCylinder, physicalHead));

            FluxSourceIterator fluxSourceIterator =
                    fluxSourceIteratorHolder.getIterator(FluxReadParameters
                            .builder()
                            .setCylinder(physicalCylinder)
                            .setHead(physicalHead)
                            .setSyncWithIndex(getConfig().getDrive().getSyncWithIndex())
                            .setReadTimeNs(getConfig().getDrive().getRevolutions() *
                                    getDiskRotationalPeriodNs())
                            .setHardSectorThresholdNs(getConfig()
                                    .getDrive()
                                    .getHardSectorThresholdNs())
                            .build());
            if (!fluxSourceIterator.hasNext())
                continue;

            Fluxmap fluxmap = fluxSourceIterator.next();
            Logger.log(new EndReadOperationLogMessage());
            Logger.logf("%d ms in %d bytes", (int) (fluxmap.durationNs() / 1e6), fluxmap.bytes());

            Track flux = getDecoder().decodeToSectors(fluxmap, ptl);
            flux.normalisedSectors = collectSectors(flux.allSectors);
            tracks.add(flux);

            /* Decode what we've got so far. */

            CombinationResult cr = combineRecordAndSectors(tracks, ltl);
            rgr.combinedSectors = cr.sectors;
            if (cr.result == BadSectorsState.HAS_NO_BAD_SECTORS)
            {
                /* We have all necessary sectors, so can stop here. */
                rgr.result = ReadResult.GOOD_READ;
                if (getConfig().getDecoder().getSkipUnnecessaryTracks())
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

    private void readAndDecodeTrack(
            LogicalTrackLayout ltl,
            List<Track> tracks,
            List<Sector> combinedSectors)
    {
        Common.FluxSourceIteratorHolder fluxSourceIteratorHolder =
                new Common.FluxSourceIteratorHolder(getFluxSource());
        int retriesRemaining = getConfig().getDecoder().getRetries();
        for (; ; )
        {
            ReadGroupResult rgr = readGroup(fluxSourceIteratorHolder, ltl, tracks);
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

            if (getFluxSource().isHardware())
            {
                adjustTrackOnError(ltl.physicalCylinder);
                Logger.logf("retrying; %d retries remaining", retriesRemaining);
                retriesRemaining--;
            }
        }
    }

    public void readDisk(Disk disk)
    {
        FluxSinkFactory outputFluxSinkFactory = null;
        if (getConfig().getDecoder().hasCopyFluxTo())
        {
            ConfigProto modifiedConfig = getConfig()
                    .toBuilder()
                    .setFluxSink(getConfig().getDecoder().getCopyFluxTo())
                    .build();
            outputFluxSinkFactory = FluxSinkFactory.create(
                    modifiedConfig, () -> {
                        throw new FluxEngineException("you can't copy flux to a hardware device");
                    });
        }

        Map<CylinderHead, List<Track>> tracksByLogicalLocation = new HashMap<>();
        for (Map.Entry<CylinderHead, Track> entry : disk.tracksByPhysicalLocation.entries())
        {
            Track track = entry.getValue();
            tracksByLogicalLocation
                    .computeIfAbsent(
                            new CylinderHead(track.ltl.logicalCylinder, track.ltl.logicalHead),
                            k -> new ArrayList<>())
                    .add(track);
        }

        Logger.log(new BeginOperationLogMessage("Reading and decoding disk"));

        disk.diskLayout = getDiskLayout();
        disk.rotationalPeriodNs = getDiskRotationalPeriodNs();
        Logger.log(new DiskUpdateLogMessage(disk));

        try (FluxSink outputFluxSink = outputFluxSinkFactory != null ?
                outputFluxSinkFactory.create() :
                null)
        {
            int index = 0;
            for (Map.Entry<CylinderHead, LogicalTrackLayout> entry :
                    getDiskLayout().layoutByLogicalLocation.entrySet())
            {
                CylinderHead logicalLocation = entry.getKey();
                LogicalTrackLayout ltl = entry.getValue();
                Logger.log(new OperationProgressLogMessage(
                        index * 100 / getDiskLayout().layoutByLogicalLocation.size()));
                index++;

                Common.testForEmergencyStop();

                List<Track> trackFluxes = tracksByLogicalLocation.computeIfAbsent(
                        logicalLocation,
                        k -> new ArrayList<>());
                List<Sector> trackSectors = new ArrayList<>();
                readAndDecodeTrack(ltl, trackFluxes, trackSectors);

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

                if (getConfig().getDecoder().getDumpRecords())
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

                if (getConfig().getDecoder().getDumpSectors())
                {
                    List<Sector> sectors = collectSectors(trackSectors, false);
                    sectors.sort(Comparator
                            .comparing((Sector s) -> s.location.logicalCylinder())
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
                Logger.log(new TrackReadLogMessage(trackFluxes, trackSectors));

                List<Sector> allSectors = new ArrayList<>();
                for (Sector sector : disk.sectorsByPhysicalLocation.values())
                    allSectors.add(sector);
                allSectors = collectSectors(allSectors);
                disk.image = new Image(allSectors);

                /* Log a _copy_ of the disk structure so that the logger
                 * doesn't see the disk get mutated in subsequent reads. */
                Logger.log(new DiskUpdateLogMessage(new Disk(disk)));
            }
        }

        if (disk.image == null)
            disk.image = new Image();

        Logger.log(new EndOperationLogMessage("Read complete"));
    }

    public Disk readDisk()
    {
        Disk disk = new Disk();
        readDisk(disk);

        ImageWriter writer = getImageWriter();
        writer.printMap(disk.image);
        if (getConfig().getDecoder().hasWriteCsvTo())
            writer.writeCsv(disk.image, getConfig().getDecoder().getWriteCsvTo());
        writer.writeImage(disk.image);

        return disk;
    }

    private void writeTracks(
            Function<LogicalTrackLayout, Fluxmap> producer,
            Predicate<LogicalTrackLayout> verifier,
            List<CylinderHead> logicalLocations)
    {
        Logger.log(new BeginOperationLogMessage("Encoding and writing to disk"));

        getDiskRotationalPeriodNs();
        try (FluxSink fluxSink = getFluxSinkFactory().create())
        {
            int index = 0;
            for (CylinderHead ch : logicalLocations)
            {
                Logger.log(new OperationProgressLogMessage(index * 100 / logicalLocations.size()));
                index++;

                Common.testForEmergencyStop();

                LogicalTrackLayout ltl = getDiskLayout().layoutByLogicalLocation.get(ch);
                int retriesRemaining = getConfig().getDecoder().getRetries();
                for (; ; )
                {
                    for (int offset = 0; offset < ltl.groupSize;
                         offset += getDiskLayout().headWidth)
                    {
                        int physicalCylinder = ltl.physicalCylinder + offset;
                        int physicalHead = ltl.physicalHead;

                        Logger.log(new BeginWriteOperationLogMessage(
                                physicalCylinder,
                                ltl.physicalHead));

                        boolean erase = false;
                        if (offset == getConfig().getDrive().getGroupOffset())
                        {
                            Fluxmap fluxmap = producer.apply(ltl);
                            if (fluxmap == null)
                                erase = true;
                            else
                            {
                                fluxSink.addFlux(physicalCylinder, physicalHead, fluxmap);
                                Logger.logf(
                                        "writing %d ms in %d bytes",
                                        (int) (fluxmap.durationNs() / 1e6),
                                        fluxmap.bytes());
                            }
                        } else
                            erase = true;

                        if (erase)
                        {
                            /* Erase this track rather than writing. */

                            Fluxmap blank = new Fluxmap();
                            fluxSink.addFlux(physicalCylinder, physicalHead, blank);
                            Logger.logf("erased");
                        }

                        Logger.log(new EndWriteOperationLogMessage());
                    }

                    if (verifier.test(ltl))
                        break;

                    if (retriesRemaining == 0)
                        throw new FluxEngineException("fatal error on write");

                    Logger.logf("retrying; %d retries remaining", retriesRemaining);
                    retriesRemaining--;
                }
            }
        }

        Logger.log(new EndOperationLogMessage("Write complete"));
    }

    public void rawWrite()
    {
        writeTracks(
                ltl -> {
                    FluxSourceIterator iterator = getFluxSource().readFlux(FluxReadParameters
                            .builder()
                            .setCylinder(ltl.physicalCylinder)
                            .setHead(ltl.physicalHead)
                            .build());
                    if (!iterator.hasNext())
                        return null;
                    return iterator.next();
                }, ltl -> true, getDiskLayout().logicalLocations);
    }

    private void writeTracks(
            Disk disk,
            Function<LogicalTrackLayout, Fluxmap> producer,
            Predicate<LogicalTrackLayout> verifier,
            ImmutableCollection<CylinderHead> logicalLocations)
    {
        Logger.log(new BeginOperationLogMessage("Encoding and writing to disk"));

        getDiskRotationalPeriodNs();
        try (FluxSink fluxSink = getFluxSinkFactory().create())
        {
            int index = 0;
            for (CylinderHead ch : logicalLocations)
            {
                Logger.log(new OperationProgressLogMessage(index * 100 / logicalLocations.size()));
                index++;

                Common.testForEmergencyStop();

                LogicalTrackLayout ltl = getDiskLayout().layoutByLogicalLocation.get(ch);
                int retriesRemaining = getConfig().getDecoder().getRetries();
                for (; ; )
                {
                    for (int offset = 0; offset < ltl.groupSize;
                         offset += getDiskLayout().headWidth)
                    {
                        int physicalCylinder = ltl.physicalCylinder + offset;
                        int physicalHead = ltl.physicalHead;
                        Track track = findPhysicalTrack(disk, ltl, physicalCylinder, physicalHead);
                        track.ltl = ltl;

                        Logger.log(new BeginWriteOperationLogMessage(
                                physicalCylinder,
                                ltl.physicalHead));

                        boolean erase = false;
                        if (offset == getConfig().getDrive().getGroupOffset())
                        {
                            Fluxmap fluxmap = producer.apply(ltl);
                            if (fluxmap == null)
                                erase = true;
                            else
                            {
                                track.fluxmap = fluxmap;

                                /* Log a _copy_ of the disk structure so that the logger
                                 * doesn't see the disk get mutated in subsequent reads. */
                                Logger.log(new DiskUpdateLogMessage(new Disk(disk)));

                                fluxSink.addFlux(physicalCylinder, physicalHead, fluxmap);
                                Logger.logf(
                                        "writing %d ms in %d bytes",
                                        (int) (fluxmap.durationNs() / 1e6),
                                        fluxmap.bytes());
                            }
                        } else
                            erase = true;

                        if (erase)
                        {
                            /* Erase this track rather than writing. */

                            Fluxmap blank = new Fluxmap();
                            track.fluxmap = blank;

                            /* Log a _copy_ of the disk structure so that the logger
                             * doesn't see the disk get mutated in subsequent reads. */
                            Logger.log(new DiskUpdateLogMessage(new Disk(disk)));

                            fluxSink.addFlux(physicalCylinder, physicalHead, blank);
                            Logger.logf("erased");
                        }

                        Logger.log(new EndWriteOperationLogMessage());
                    }

                    if (verifier.test(ltl))
                        break;

                    if (retriesRemaining == 0)
                        throw new FluxEngineException("fatal error on write");

                    Logger.logf("retrying; %d retries remaining", retriesRemaining);
                    retriesRemaining--;
                }
            }
        }

        Logger.log(new EndOperationLogMessage("Write complete"));
    }

    private Track findPhysicalTrack(
            Disk disk,
            LogicalTrackLayout ltl,
            int physicalCylinder,
            int physicalHead)
    {
        CylinderHead pch = new CylinderHead(physicalCylinder, physicalHead);
        List<Track> tracks = disk.tracksByPhysicalLocation.get(pch);
        Track track;
        if (tracks.isEmpty())
        {
            track = new Track();
            track.ltl = ltl;
            disk.tracksByPhysicalLocation.put(pch, track);
        } else
            track = tracks.getFirst();
        return track;
    }

    private void writeTracks(Disk disk, ImmutableCollection<CylinderHead> logicalLocations)
    {
        writeTracks(
                disk, ltl -> {
                    ImmutableList<Sector> sectors = getEncoder().collectSectors(ltl, disk.image);
                    return getEncoder().encode(ltl, sectors, disk.image);
                }, ltl -> true, logicalLocations);
    }

    private void writeTracksAndVerify(Disk disk, ImmutableCollection<CylinderHead> logicalLocations)
    {
        writeTracks(
                disk, ltl -> {
                    List<Sector> sectors = getEncoder().collectSectors(ltl, disk.image);
                    return getEncoder().encode(ltl, sectors, disk.image);
                }, ltl -> {
                    Common.FluxSourceIteratorHolder fluxSourceIteratorHolder =
                            new Common.FluxSourceIteratorHolder(getFluxSource());

                    List<Track> tracks = new ArrayList<>();
                    ReadGroupResult rgr = readGroup(fluxSourceIteratorHolder, ltl, tracks);

                    CylinderHead pch = new CylinderHead(ltl.physicalCylinder, ltl.physicalHead);
                    disk.tracksByPhysicalLocation.removeAll(pch);
                    disk.tracksByPhysicalLocation.putAll(pch, tracks);
                    disk.sectorsByPhysicalLocation.removeAll(pch);
                    disk.sectorsByPhysicalLocation.putAll(pch, rgr.combinedSectors);

                    /* Log a _copy_ of the disk structure so that the logger
                     * doesn't see the disk get mutated in subsequent reads. */
                    Logger.log(new DiskUpdateLogMessage(new Disk(disk)));

                    if (rgr.result != ReadResult.GOOD_READ)
                    {
                        if (getFluxSinkFactory().isHardware())
                            adjustTrackOnError(ltl.physicalCylinder);
                        Logger.logf("bad read");
                        return false;
                    }

                    Image wanted = new Image();
                    for (Sector sector : getEncoder().collectSectors(ltl, disk.image))
                        wanted.put(
                                sector.location.logicalCylinder(),
                                sector.location.logicalHead(),
                                sector.location.logicalSector()).data = sector.data;

                    for (Sector sector : rgr.combinedSectors)
                    {
                        Sector s = wanted.get(
                                sector.location.logicalCylinder(),
                                sector.location.logicalHead(),
                                sector.location.logicalSector());
                        if (s == null)
                        {
                            Logger.logf("spurious sector on verify");
                            return false;
                        }
                        if (!s.data.equals(sector.data.slice(0, s.data.size())))
                        {
                            Logger.logf("data mismatch on verify");
                            return false;
                        }
                        wanted.erase(
                                sector.location.logicalCylinder(),
                                sector.location.logicalHead(),
                                sector.location.logicalSector());
                    }
                    if (!wanted.empty())
                    {
                        Logger.logf("missing sector on verify");
                        return false;
                    }
                    return true;
                }, logicalLocations);
    }

    public void writeDisk(Disk disk, ImmutableCollection<CylinderHead> logicalLocations)
    {
        if (getConfig().getVerifyWrites())
            writeTracksAndVerify(disk, logicalLocations);
        else
            writeTracks(disk, logicalLocations);
    }

    public void writeDisk(Disk disk)
    {
        writeDisk(disk, disk.diskLayout.layoutByLogicalLocation.keySet());
    }

    enum ReadResult
    {
        GOOD_READ, BAD_AND_CAN_RETRY, BAD_AND_CAN_NOT_RETRY
    }

    enum BadSectorsState
    {
        HAS_NO_BAD_SECTORS, HAS_BAD_SECTORS
    }

    static class CombinationResult
    {
        BadSectorsState result;
        List<Sector> sectors;
    }

    static class ReadGroupResult
    {
        ReadResult result;
        List<Sector> combinedSectors;
    }
}
