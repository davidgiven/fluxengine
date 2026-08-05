package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.usb.UsbFactory;
import picocli.CommandLine.Command;
import javax.inject.Inject;

/**
 * Test USB bulk transfer bandwidth, modelled after src/fe-testbandwidth.cc.
 */
@Command(name = "bandwidth", description = "Test USB bulk transfer bandwidth")
public class TestBandwidthCommand implements Runnable
{
    private final UsbFactory usbFactory;

    @Inject
    TestBandwidthCommand(UsbFactory usbFactory)
    {
        this.usbFactory = usbFactory;
    }

    @Override
    public void run()
    {
        /* The C++ acquires the device via getUsb(), which isn't wired up in
         * the Java port yet, so the bulk tests are commented out until device
         * selection is available. */
        // AbstractUsbDevice device = getUsb();
        // device.testBulkWrite();
        // device.testBulkRead();
    }
}
