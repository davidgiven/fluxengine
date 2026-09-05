package com.cowlark.fluxengine.usb;

/**
 * A set of FDD bus voltage readings, ported from struct voltages_frame in
 * protocol.h.
 */
public class VoltageMeasurements
{
    public Voltages inputBothOff;
    public Voltages inputDrive0Selected;
    public Voltages inputDrive1Selected;
    public Voltages inputDrive0Running;
    public Voltages inputDrive1Running;
    public Voltages outputBothOff;
    public Voltages outputDrive0Selected;
    public Voltages outputDrive1Selected;
    public Voltages outputDrive0Running;
    public Voltages outputDrive1Running;
}
