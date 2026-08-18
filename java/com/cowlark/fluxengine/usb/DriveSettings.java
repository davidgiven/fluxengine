package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;

public class DriveSettings
{
    public int drive = 0;
    public int seekPosition = 0;
    public boolean highDensity = true;
    public int side = 0;
    public double hardSectorThresholdNs = 0.0;
    public int hardSectorCount = 0;
    public boolean synced = false;

    public DriveSettings()
    {
    }

    public DriveSettings(ConfigProto configProto)
    {
        drive = configProto.getDrive().getDrive();
        seekPosition = 0;
        highDensity = configProto.getDrive().getHighDensity();
        side = 0;
        hardSectorThresholdNs = configProto.getDrive().getHardSectorThresholdNs();
        hardSectorCount = configProto.getDrive().getHardSectorCount();
        synced = configProto.getDrive().getSyncWithIndex();
    }
}
