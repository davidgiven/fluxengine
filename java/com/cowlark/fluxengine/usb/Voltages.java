package com.cowlark.fluxengine.usb;

import lombok.Builder;

/**
 * Voltage readings, ported from struct voltages in protocol.h.
 */
@Builder(setterPrefix = "set")
public record Voltages(int logic0Mv, int logic1Mv)
{
}
