package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import java.util.ArrayList;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;

/**
 * Writes images to disks, ported from lib/algorithms/readerwriter.cc.
 */
public final class Writer
{
    private Writer()
    {
    }

    private static void writeTracks(ConfigProto config,
                                    DiskLayout diskLayout,
                                    FluxSinkFactory fluxSinkFactory,
                                    Function<LogicalTrackLayout, Fluxmap> producer,
                                    Predicate<LogicalTrackLayout> verifier,
                                    List<CylinderHead> logicalLocations)
    {
        Logger.log(new LogMessage.BeginOperationLogMessage("Encoding and writing to disk"));

        if (fluxSinkFactory.isHardware())
            Common.measureDiskRotation(config);
        try (FluxSink fluxSink = fluxSinkFactory.create())
        {
            int index = 0;
            for (CylinderHead ch : logicalLocations)
            {
                Logger.log(new LogMessage.OperationProgressLogMessage(
                        index * 100 / logicalLocations.size()));
                index++;

                Common.testForEmergencyStop();

                LogicalTrackLayout ltl = diskLayout.layoutByLogicalLocation.get(ch);
                int retriesRemaining = config.getDecoder().getRetries();
                for (; ; )
                {
                    for (int offset = 0; offset < ltl.groupSize; offset += diskLayout.headWidth)
                    {
                        int physicalCylinder = ltl.physicalCylinder + offset;
                        int physicalHead = ltl.physicalHead;

                        Logger.log(new LogMessage.BeginWriteOperationLogMessage(
                                physicalCylinder,
                                ltl.physicalHead));

                        boolean erase = false;
                        if (offset == config.getDrive().getGroupOffset())
                        {
                            Fluxmap fluxmap = producer.apply(ltl);
                            if (fluxmap == null)
                                erase = true;
                            else
                            {
                                fluxSink.addFlux(physicalCylinder, physicalHead, fluxmap);
                                Logger.logf(
                                        "writing %d ms in %d bytes",
                                        (int) (fluxmap.duration() / 1e6),
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

                        Logger.log(new LogMessage.EndWriteOperationLogMessage());
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

        Logger.log(new LogMessage.EndOperationLogMessage("Write complete"));
    }

    private static void writeTracks(ConfigProto config,
                                    DiskLayout diskLayout,
                                    FluxSinkFactory fluxSinkFactory,
                                    Encoder encoder,
                                    Image image,
                                    List<CylinderHead> chs)
    {
        writeTracks(
                config, diskLayout, fluxSinkFactory, ltl -> {
                    List<Sector> sectors = encoder.collectSectors(ltl, image);
                    return encoder.encode(ltl, sectors, image);
                }, ltl -> true, chs);
    }

    private static void writeTracksAndVerify(ConfigProto config,
                                             DiskLayout diskLayout,
                                             FluxSinkFactory fluxSinkFactory,
                                             Encoder encoder,
                                             FluxSource fluxSource,
                                             Decoder decoder,
                                             Image image,
                                             List<CylinderHead> chs)
    {
        writeTracks(
                config, diskLayout, fluxSinkFactory, ltl -> {
                    List<Sector> sectors = encoder.collectSectors(ltl, image);
                    return encoder.encode(ltl, sectors, image);
                }, ltl -> {
                    Common.FluxSourceIteratorHolder fluxSourceIteratorHolder =
                            new Common.FluxSourceIteratorHolder(fluxSource);
                    List<Track> tracks = new ArrayList<>();
                    Reader.ReadGroupResult rgr = Reader.readGroup(
                            diskLayout,
                            fluxSourceIteratorHolder,
                            ltl,
                            tracks,
                            decoder,
                            config);

                    if (rgr.result != Reader.ReadResult.GOOD_READ)
                    {
                        Common.adjustTrackOnError(fluxSource, ltl.physicalCylinder, config);
                        Logger.logf("bad read");
                        return false;
                    }

                    Image wanted = new Image();
                    for (Sector sector : encoder.collectSectors(ltl, image))
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
                }, chs);
    }

    public static void writeDiskCommand(ConfigProto config,
                                        DiskLayout diskLayout,
                                        Image image,
                                        Encoder encoder,
                                        FluxSinkFactory fluxSinkFactory,
                                        Decoder decoder,
                                        FluxSource fluxSource,
                                        List<CylinderHead> physicalLocations)
    {
        List<CylinderHead> chs = new ArrayList<>(diskLayout.layoutByLogicalLocation.keySet());
        if (fluxSource != null && decoder != null)
            writeTracksAndVerify(
                    config,
                    diskLayout,
                    fluxSinkFactory,
                    encoder,
                    fluxSource,
                    decoder,
                    image,
                    chs);
        else
            writeTracks(config, diskLayout, fluxSinkFactory, encoder, image, chs);
    }

    public static void writeDiskCommand(ConfigProto config,
                                        DiskLayout diskLayout,
                                        Image image,
                                        Encoder encoder,
                                        FluxSinkFactory fluxSinkFactory,
                                        Decoder decoder,
                                        FluxSource fluxSource)
    {
        writeDiskCommand(
                config,
                diskLayout,
                image,
                encoder,
                fluxSinkFactory,
                decoder,
                fluxSource,
                new ArrayList<>(diskLayout.layoutByLogicalLocation.keySet()));
    }

    public static void writeRawDiskCommand(ConfigProto config,
                                           DiskLayout diskLayout,
                                           FluxSource fluxSource,
                                           FluxSinkFactory fluxSinkFactory)
    {
        writeTracks(
                config, diskLayout, fluxSinkFactory, ltl -> {
                    FluxSourceIterator iterator =
                            fluxSource.readFlux(ltl.physicalCylinder, ltl.physicalHead);
                    if (!iterator.hasNext())
                        return null;
                    return iterator.next();
                }, ltl -> true, diskLayout.logicalLocations);
    }
}
