package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.LogMessage.BeginOperationLogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableSet;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;

public class WriteOperation extends RawWriteOperation
{
    public WriteOperation(ConfigProto configProto)
    {
        super(configProto);
    }

    private void writeTracks(Function<LogicalTrackLayout, Fluxmap> producer,
                             Predicate<LogicalTrackLayout> verifier,
                             ImmutableSet<CylinderHead> logicalLocations)
    {
        Logger.log(new BeginOperationLogMessage("Encoding and writing to disk"));

        getDiskRotationalPeriodNs();
        try (FluxSink fluxSink = getFluxSinkFactory().create())
        {
            int index = 0;
            for (CylinderHead ch : logicalLocations)
            {
                Logger.log(new LogMessage.OperationProgressLogMessage(
                        index * 100 / logicalLocations.size()));
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

                        Logger.log(new LogMessage.BeginWriteOperationLogMessage(
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

    private void writeTracks(Image image, ImmutableSet<CylinderHead> chs)
    {
        writeTracks(
                ltl -> {
                    ImmutableList<Sector> sectors = getEncoder().collectSectors(ltl, image);
                    return getEncoder().encode(ltl, sectors, image);
                }, ltl -> true, chs);
    }

    private void writeTracksAndVerify(Image image, ImmutableSet<CylinderHead> chs)
    {
        writeTracks(
                ltl -> {
                    List<Sector> sectors = getEncoder().collectSectors(ltl, image);
                    return getEncoder().encode(ltl, sectors, image);
                }, ltl -> {
                    Common.FluxSourceIteratorHolder fluxSourceIteratorHolder =
                            new Common.FluxSourceIteratorHolder(getFluxSource());
                    List<Track> tracks = new ArrayList<>();
                    ReadGroupResult rgr = readGroup(fluxSourceIteratorHolder, ltl, tracks);

                    if (rgr.result != ReadOperation.ReadResult.GOOD_READ)
                    {
                        adjustTrackOnError(ltl.physicalCylinder);
                        Logger.logf("bad read");
                        return false;
                    }

                    Image wanted = new Image();
                    for (Sector sector : getEncoder().collectSectors(ltl, image))
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

    public void writeDiskCommand(Image image, Collection<CylinderHead> physicalLocations)
    {
        ImmutableSet<CylinderHead> chs = getDiskLayout().layoutByLogicalLocation.keySet();
        if (getConfig().getVerifyWrites())
            writeTracksAndVerify(image, chs);
        else
            writeTracks(image, chs);
    }

    public void writeDiskCommand(Image image)
    {
        writeDiskCommand(image, getDiskLayout().layoutByLogicalLocation.keySet());
    }
}
