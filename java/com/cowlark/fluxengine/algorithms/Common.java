package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.fluxsource.FluxSourceIterator;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import java.util.HashMap;
import java.util.Map;

class Common
{
    static double getRotationalPeriodFromConfig(ConfigProto config)
    {
        return config.getDrive().getRotationalPeriodMs() * 1e6;
    }

    static double measureDiskRotation(ConfigProto config)
    {
        Logger.log(new LogMessage.BeginSpeedOperationLogMessage());

        double oneRevolution = getRotationalPeriodFromConfig(config);
        if (oneRevolution == 0)
        {
            UsbDevice device = UsbFactory.reconnect(config);
            device.setDrive(
                    config.getDrive().getDrive(),
                    config.getDrive().getHighDensity(),
                    config.getDrive().getIndexMode().getNumber());

            Logger.log(new LogMessage.BeginOperationLogMessage("Measuring drive rotational speed"));
            int retries = 5;
            do
            {
                oneRevolution = device.getRotationalPeriod(config.getDrive().getHardSectorCount());
                retries--;
            } while ((oneRevolution == 0) && (retries > 0));
            Logger.log(new LogMessage.EndOperationLogMessage(""));
        }

        if (oneRevolution == 0)
            throw new FluxEngineException("Failed\nIs a disk in the drive?");

        Logger.log(new LogMessage.EndSpeedOperationLogMessage(oneRevolution));
        return oneRevolution;
    }

    static void testForEmergencyStop()
    {
    }

    static void adjustTrackOnError(FluxSource fluxSource, int baseTrack, ConfigProto config)
    {
        switch (config.getDrive().getErrorBehaviour())
        {
            case NOTHING:
                break;

            case RECALIBRATE:
                fluxSource.recalibrate();
                break;

            case JIGGLE:
                if (baseTrack > 0)
                    fluxSource.seek(baseTrack - 1);
                else
                    fluxSource.seek(baseTrack + 1);
                break;
        }
    }

    static class FluxSourceIteratorHolder
    {
        private final FluxSource fluxSource;
        private final Map<CylinderHead, FluxSourceIterator> cache = new HashMap<>();

        FluxSourceIteratorHolder(FluxSource fluxSource)
        {
            this.fluxSource = fluxSource;
        }

        FluxSourceIterator getIterator(int physicalCylinder, int head)
        {
            CylinderHead key = new CylinderHead(physicalCylinder, head);
            FluxSourceIterator it = cache.get(key);
            if (it == null)
            {
                it = fluxSource.readFlux(physicalCylinder, head);
                cache.put(key, it);
            }
            return it;
        }
    }
}
