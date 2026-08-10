package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.fluxsink.FluxSink;
import com.cowlark.fluxengine.fluxsource.FluxReadParameters;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import java.util.List;
import java.util.function.Function;
import java.util.function.Predicate;

public class RawWriteOperation extends ReadOperation
{
    public RawWriteOperation(ConfigProto configProto)
    {
        super(configProto);
    }

    private void writeTracks(Function<LogicalTrackLayout, Fluxmap> producer,
                             Predicate<LogicalTrackLayout> verifier,
                             List<CylinderHead> logicalLocations)
    {
        Logger.log(new LogMessage.BeginOperationLogMessage("Encoding and writing to disk"));

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

    public void rawWrite()
    {
        writeTracks(
                ltl -> {
                    FluxSourceIterator iterator =
                            getFluxSource().readFlux(FluxReadParameters.builder()
                                    .setCylinder(ltl.physicalCylinder)
                                    .setHead(ltl.physicalHead)
                                    .build());
                    if (!iterator.hasNext())
                        return null;
                    return iterator.next();
                }, ltl -> true, getDiskLayout().logicalLocations);
    }
}
