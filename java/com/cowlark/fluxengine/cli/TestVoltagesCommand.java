package com.cowlark.fluxengine.cli;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.cowlark.fluxengine.usb.Voltages;
import com.google.common.collect.ImmutableList;

/**
 * Measure the FDD bus voltages, modelled after src/fe-testvoltages.cc.
 */
public class TestVoltagesCommand implements Command
{
    /* The device fills the array in the order of the C++ voltages_frame
     * struct. */
    private static final int INPUT_BOTH_OFF = 0;
    private static final int INPUT_DRIVE_0_SELECTED = 1;
    private static final int INPUT_DRIVE_1_SELECTED = 2;
    private static final int INPUT_DRIVE_0_RUNNING = 3;
    private static final int INPUT_DRIVE_1_RUNNING = 4;
    private static final int OUTPUT_BOTH_OFF = 5;
    private static final int OUTPUT_DRIVE_0_SELECTED = 6;
    private static final int OUTPUT_DRIVE_1_SELECTED = 7;
    private static final int OUTPUT_DRIVE_0_RUNNING = 8;
    private static final int OUTPUT_DRIVE_1_RUNNING = 9;

    @Override
    public String getHelp()
    {
        return "Measures the FDD bus voltages.";
    }

    @Override
    public void run(ImmutableList<String> args)
    {
        ConfigProto config = new ConfigBuilder().fromFlags(args).build();

        Voltages[] voltages = new Voltages[10];
        UsbDevice device = UsbFactory.connect(config);
        device.measureVoltages(voltages);

        System.out.printf("""
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
            displayVoltages(voltages[OUTPUT_BOTH_OFF]),
            displayVoltages(voltages[OUTPUT_DRIVE_0_SELECTED]),
            displayVoltages(voltages[OUTPUT_DRIVE_1_SELECTED]),
            displayVoltages(voltages[OUTPUT_DRIVE_0_RUNNING]),
            displayVoltages(voltages[OUTPUT_DRIVE_1_RUNNING]),
            displayVoltages(voltages[INPUT_BOTH_OFF]),
            displayVoltages(voltages[INPUT_DRIVE_0_SELECTED]),
            displayVoltages(voltages[INPUT_DRIVE_1_SELECTED]),
            displayVoltages(voltages[INPUT_DRIVE_0_RUNNING]),
            displayVoltages(voltages[INPUT_DRIVE_1_RUNNING]));
    }

    private static String displayVoltages(Voltages v)
    {
        return String.format("      Logic 1 / 0:  %.2fV / %.2fV\n",
            v.logic0Mv() / 1000.0, v.logic1Mv() / 1000.0);
    }
}
