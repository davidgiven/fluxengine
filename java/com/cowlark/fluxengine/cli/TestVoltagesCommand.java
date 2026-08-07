package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.cowlark.fluxengine.usb.VoltageMeasurements;
import com.cowlark.fluxengine.usb.Voltages;
import com.google.common.collect.ImmutableList;

/**
 * Measure the FDD bus voltages, modelled after src/fe-testvoltages.cc.
 */
public class TestVoltagesCommand implements Command
{
    private static String displayVoltages(Voltages v)
    {
        return String.format(
                "      Logic 1 / 0:  %.2fV / %.2fV\n",
                v.logic0Mv() / 1000.0,
                v.logic1Mv() / 1000.0);
    }

    @Override
    public String getHelp()
    {
        return "Measures the FDD bus voltages.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder().fromFlags(args).build();

        UsbDevice device = UsbFactory.connect(config);
        VoltageMeasurements voltages = device.measureVoltages();

        System.out.printf(
                """
                        Output voltages:
                          Both drives deselected
                        %s  Drive 0 selected
                        %s  Drive 1 selected
                        %s  Drive 0 running
                        %s  Drive 1 running
                        %sInput voltages:
                          Both drives deselected
                        %s  Drive 0 selected
                        %s  Drive 1 selected
                        %s  Drive 0 running
                        %s  Drive 1 running
                        %s""",
                displayVoltages(voltages.outputBothOff),
                displayVoltages(voltages.outputDrive0Selected),
                displayVoltages(voltages.outputDrive1Selected),
                displayVoltages(voltages.outputDrive0Running),
                displayVoltages(voltages.outputDrive1Running),
                displayVoltages(voltages.inputBothOff),
                displayVoltages(voltages.inputDrive0Selected),
                displayVoltages(voltages.inputDrive1Selected),
                displayVoltages(voltages.inputDrive0Running),
                displayVoltages(voltages.inputDrive1Running));
    }
}
