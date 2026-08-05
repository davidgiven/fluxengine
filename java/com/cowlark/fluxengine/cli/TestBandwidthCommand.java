package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.wiring.FluxEngineComponent;
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
        FluxEngineComponent component = FluxEngineComponent.create(unmatchedArguments());

        /* The C++ acquires the device via getUsb(), which isn't wired up in
         * the Java port yet, so the bulk tests are commented out until device
         * selection is available. */
        // UsbDevice device = component.usbComponentFactory().create().usbFactory().connect();
        // device.testBulkWrite();
        // device.testBulkRead();
    }
}
