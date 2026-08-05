package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.Config;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.cowlark.fluxengine.usb.UsbDevice;
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
        UsbFactory usbFactory =
            new UsbFactory(new Config(ImmutableList.copyOf(unmatchedArguments())));
        UsbDevice device = usbFactory.connect();

        /* The C++ acquires the device via getUsb(), which isn't wired up in
         * the Java port yet, so the bulk tests are commented out until device
         * selection is available. */
        // device.testBulkWrite();
        // device.testBulkRead();
    }
}
