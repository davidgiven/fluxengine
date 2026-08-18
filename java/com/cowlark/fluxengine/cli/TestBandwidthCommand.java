package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;

/**
 * Test USB bulk transfer bandwidth, modelled after src/fe-testbandwidth.cc.
 */
public class TestBandwidthCommand implements Command
{
    @Override
    public String getHelp()
    {
        return "Measures your USB bandwidth.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder().fromFlags(args).build();

        UsbDevice device = UsbFactory.getConnection(config);
        device.testBulkWrite();
        device.testBulkRead();
    }
}
