package com.cowlark.fluxengine.external;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class CrcTest
{
    /* The standard CRC check value: the result over the ASCII string
     * "123456789". */
    private static final Bytes CHECK = Bytes.of(0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39);

    @Test
    public void crc16()
    {
        /* CRC-16/CCITT-FALSE. */
        assertThat(Crc.crc16(Crc.CCITT_POLY, CHECK)).isEqualTo(0x29b1);

        /* CRC-16/XMODEM. */
        assertThat(Crc.crc16(Crc.CCITT_POLY, 0x0000, CHECK)).isEqualTo(0x31c3);

        /* TD0 imagereader polynomial. */
        assertThat(Crc.crc16(0xa097, 0x0000, CHECK)).isEqualTo(0x0fb3);

        /* The F85 decoder uses CCITT with non-standard init values. */
        assertThat(Crc.crc16(Crc.CCITT_POLY, 0xef21, CHECK)).isEqualTo(0xd2bb);
        assertThat(Crc.crc16(Crc.CCITT_POLY, 0xbf84, CHECK)).isEqualTo(0x10cb);
    }

    @Test
    public void crc16ref()
    {
        /* CRC-16/MODBUS. */
        assertThat(Crc.crc16ref(Crc.MODBUS_POLY_REF, CHECK)).isEqualTo(0x4b37);
        assertThat(Crc.crc16ref(Crc.MODBUS_POLY_REF, 0x0000, CHECK)).isEqualTo(0xbb3d);
    }

    @Test
    public void crc16Empty()
    {
        /* An empty input leaves the CRC at its init value. */
        assertThat(Crc.crc16(Crc.CCITT_POLY, Bytes.of())).isEqualTo(0xffff);
        assertThat(Crc.crc16ref(Crc.MODBUS_POLY_REF, Bytes.of())).isEqualTo(0xffff);
    }

    @Test
    public void crc16SingleByte()
    {
        assertThat(Crc.crc16(Crc.CCITT_POLY, Bytes.of(0x00))).isEqualTo(0xe1f0);
        assertThat(Crc.crc16(Crc.CCITT_POLY, Bytes.of(0x01))).isEqualTo(0xf1d1);
        assertThat(Crc.crc16ref(Crc.MODBUS_POLY_REF, Bytes.of(0x00))).isEqualTo(0x40bf);
        assertThat(Crc.crc16ref(Crc.MODBUS_POLY_REF, Bytes.of(0xff))).isEqualTo(0xff);
    }

    @Test
    public void sumBytes()
    {
        assertThat(Crc.sumBytes(Bytes.of(1, 2, 3, 4))).isEqualTo(10);
        assertThat(Crc.sumBytes(Bytes.of())).isEqualTo(0);
        assertThat(Crc.sumBytes(Bytes.of(0xff, 0x01))).isEqualTo(0x100);
    }

    @Test
    public void xorBytes()
    {
        assertThat(Crc.xorBytes(Bytes.of(1, 2, 3, 4))).isEqualTo(4);
        assertThat(Crc.xorBytes(Bytes.of())).isEqualTo(0);
        assertThat(Crc.xorBytes(Bytes.of(0xff, 0xff))).isEqualTo(0);
    }
}
