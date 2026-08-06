package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigFlagGroup;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.collect.ImmutableList;
import picocli.CommandLine.Command;

/**
 * Test USB bulk transfer bandwidth, modelled after src/fe-testbandwidth.cc.
 */
@Command(name = "bandwidth", description = "Test USB bulk transfer bandwidth")
public class TestBandwidthCommand extends CommandWithConfig implements Runnable
{
    @Override
    public void run()
    {
        var configGroup = new ConfigFlagGroup();
        ConfigProto config = new ConfigBuilder()
                .fromFlags(ImmutableList.of(), configGroup)
                .build();

        UsbDevice device = UsbFactory.connect(config);
        device.testBulkWrite();
        device.testBulkRead();
    }
}
