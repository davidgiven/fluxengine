package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigParser;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
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
        ConfigParser config = new ConfigParser();
        config.parse(unmatchedArguments());
        UsbDevice device = UsbFactory.connect(config.build());
        device.testBulkWrite();
        device.testBulkRead();
    }
}
