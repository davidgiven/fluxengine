package com.cowlark.fluxengine.arch.northstar;

/**
 * Constants for the North Star format, ported from arch/northstar/northstar.h.
 * <p>
 * Northstar floppies are 10-hard sectored disks with a sector format as
 * follows:
 * <p>
 * |----------------------------------|
 * | SYNC Byte  | Payload  | Checksum |
 * |------------+----------+----------|
 * | 1 (0xFB)   | 256 (SD) |    1     |
 * | 2 (0xFBFB) | 512 (DD) |          |
 * |----------------------------------|
 */
public final class Northstar
{
    public static final int NORTHSTAR_PREAMBLE_SIZE_SD = (16);
    public static final int NORTHSTAR_PREAMBLE_SIZE_DD = (32);
    public static final int NORTHSTAR_HEADER_SIZE_SD = (1);
    public static final int NORTHSTAR_HEADER_SIZE_DD = (2);
    public static final int NORTHSTAR_PAYLOAD_SIZE_SD = (256);
    public static final int NORTHSTAR_PAYLOAD_SIZE_DD = (512);
    public static final int NORTHSTAR_CHECKSUM_SIZE = (1);
    public static final int NORTHSTAR_ENCODED_SECTOR_SIZE_SD =
            (NORTHSTAR_HEADER_SIZE_SD + NORTHSTAR_PAYLOAD_SIZE_SD + NORTHSTAR_CHECKSUM_SIZE);
    public static final int NORTHSTAR_ENCODED_SECTOR_SIZE_DD =
            (NORTHSTAR_HEADER_SIZE_DD + NORTHSTAR_PAYLOAD_SIZE_DD + NORTHSTAR_CHECKSUM_SIZE);

    private Northstar()
    {
    }
}