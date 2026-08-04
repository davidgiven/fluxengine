package com.cowlark.fluxengine.usb;

/**
 * Voltage readings, ported from struct voltages in protocol.h.
 */
public record Voltages(int logic0Mv, int logic1Mv)
{
}
