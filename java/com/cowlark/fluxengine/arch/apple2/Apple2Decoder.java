package com.cowlark.fluxengine.arch.apple2;

import com.cowlark.fluxengine.apple2.Apple2DecoderProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;

/**
 * The Apple II decoder, ported from arch/apple2/decoder.cc.
 */
public class Apple2Decoder extends Decoder
{
    private static final FluxPattern SECTOR_RECORD_PATTERN =
            new FluxPattern(24, Apple2.APPLE2_SECTOR_RECORD);
    private static final FluxPattern DATA_RECORD_PATTERN =
            new FluxPattern(24, Apple2.APPLE2_DATA_RECORD);
    private static final FluxMatchers ANY_RECORD_PATTERN =
            FluxMatchers.of(SECTOR_RECORD_PATTERN, DATA_RECORD_PATTERN);
    private final Apple2DecoderProto config;

    public Apple2Decoder(DecoderProto config)
    {
        super(config);
        this.config = config.getApple2();
    }

    private static int decodeDataGcr(int gcr)
    {
        switch (gcr)
        {
            case 0x96:
                return 0x00;
            case 0x97:
                return 0x01;
            case 0x9a:
                return 0x02;
            case 0x9b:
                return 0x03;
            case 0x9d:
                return 0x04;
            case 0x9e:
                return 0x05;
            case 0x9f:
                return 0x06;
            case 0xa6:
                return 0x07;
            case 0xa7:
                return 0x08;
            case 0xab:
                return 0x09;
            case 0xac:
                return 0x0a;
            case 0xad:
                return 0x0b;
            case 0xae:
                return 0x0c;
            case 0xaf:
                return 0x0d;
            case 0xb2:
                return 0x0e;
            case 0xb3:
                return 0x0f;
            case 0xb4:
                return 0x10;
            case 0xb5:
                return 0x11;
            case 0xb6:
                return 0x12;
            case 0xb7:
                return 0x13;
            case 0xb9:
                return 0x14;
            case 0xba:
                return 0x15;
            case 0xbb:
                return 0x16;
            case 0xbc:
                return 0x17;
            case 0xbd:
                return 0x18;
            case 0xbe:
                return 0x19;
            case 0xbf:
                return 0x1a;
            case 0xcb:
                return 0x1b;
            case 0xcd:
                return 0x1c;
            case 0xce:
                return 0x1d;
            case 0xcf:
                return 0x1e;
            case 0xd3:
                return 0x1f;
            case 0xd6:
                return 0x20;
            case 0xd7:
                return 0x21;
            case 0xd9:
                return 0x22;
            case 0xda:
                return 0x23;
            case 0xdb:
                return 0x24;
            case 0xdc:
                return 0x25;
            case 0xdd:
                return 0x26;
            case 0xde:
                return 0x27;
            case 0xdf:
                return 0x28;
            case 0xe5:
                return 0x29;
            case 0xe6:
                return 0x2a;
            case 0xe7:
                return 0x2b;
            case 0xe9:
                return 0x2c;
            case 0xea:
                return 0x2d;
            case 0xeb:
                return 0x2e;
            case 0xec:
                return 0x2f;
            case 0xed:
                return 0x30;
            case 0xee:
                return 0x31;
            case 0xef:
                return 0x32;
            case 0xf2:
                return 0x33;
            case 0xf3:
                return 0x34;
            case 0xf4:
                return 0x35;
            case 0xf5:
                return 0x36;
            case 0xf6:
                return 0x37;
            case 0xf7:
                return 0x38;
            case 0xf9:
                return 0x39;
            case 0xfa:
                return 0x3a;
            case 0xfb:
                return 0x3b;
            case 0xfc:
                return 0x3c;
            case 0xfd:
                return 0x3d;
            case 0xfe:
                return 0x3e;
            case 0xff:
                return 0x3f;
            default:
                return -1;
        }
    }

    private static int combine(int word)
    {
        return (word & (word >> 7)) & 0xff;
    }

    /* This is extremely inspired by the MESS implementation, written by Nathan
     * Woods and R. Belmont:
     * https://github.com/mamedev/mame/blob/7914a6083a3b3a8c243ae6c3b8cb50b023f21e0e/src/lib
     * /formats/ap2_dsk.cpp
     */
    private static Bytes decodeCrazyData(Bytes input, Sector.Status[] status)
    {
        Bytes output = new Bytes(Apple2.APPLE2_SECTOR_LENGTH);

        int checksum = 0;
        for (int i = 0; i < Apple2.APPLE2_ENCODED_SECTOR_LENGTH; i++)
        {
            checksum ^= decodeDataGcr(input.getByte(i) & 0xff);

            if (i >= 86)
            {
                /* 6 bit */
                output.setByte(i - 86, (byte) (output.getByte(i - 86) | (checksum << 2)));
            } else
            {
                /* 3 * 2 bit */
                output.setByte(i, (byte) (((checksum >> 1) & 0x01) | ((checksum << 1) & 0x02)));
                output.setByte(i + 86,
                        (byte) (((checksum >> 3) & 0x01) | ((checksum >> 1) & 0x02)));
                if ((i + 172) < Apple2.APPLE2_SECTOR_LENGTH)
                    output.setByte(i + 172,
                            (byte) (((checksum >> 5) & 0x01) | ((checksum >> 3) & 0x02)));
            }
        }

        checksum &= 0x3f;
        int wantedchecksum =
                decodeDataGcr(input.getByte(Apple2.APPLE2_ENCODED_SECTOR_LENGTH) & 0xff);
        status[0] = (checksum == wantedchecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
        return output;
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        if (readRaw24() != Apple2.APPLE2_SECTOR_RECORD)
            return;

        /* Read header. */

        Bytes header = readRawBits(8 * 8).toBytes().slice(0, 8);
        ByteReader br = header.iterator();

        int volume = combine(br.readBe16());
        int logicalCylinder = combine(br.readBe16());
        int logicalHead = ltl.logicalHead;
        int logicalSector = combine(br.readBe16());
        int checksum = combine(br.readBe16());

        /* If the checksum is correct, upgrade the sector from MISSING to
         * DATA_MISSING in anticipation of its data record. */
        if (checksum == (volume ^ logicalCylinder ^ logicalSector))
            sector.status = Sector.Status.DATA_MISSING;

        if (logicalHead == 1)
            logicalCylinder -= config.getSideOneTrackOffset();

        /* Sanity check. */

        if (logicalCylinder > 100)
        {
            sector.status = Sector.Status.MISSING;
            return;
        }

        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);
    }

    @Override
    protected void decodeDataRecord()
    {
        /* Check ID. */

        if (readRaw24() != Apple2.APPLE2_DATA_RECORD)
            return;

        /* Read and decode data. */

        /* Sometimes there's a 1-bit gap between APPLE2_DATA_RECORD and the
         * data itself. This has been seen on real world disks such as the
         * Apple II Operating System Kit from Apple2Online. However, I haven't
         * seen it described in any of the various references.
         *
         * This extra '0' bit would not affect the real disk interface, as it
         * was a '1' reaching the top bit of a shift register that triggered a
         * byte to be available, but it affects the way the data is read here.
         *
         * While the floppies tested only seemed to need this applied to the
         * first byte of the data record, applying it consistently to all of
         * them doesn't seem to hurt, and simplifies the code.
         */

        int recordLength = Apple2.APPLE2_ENCODED_SECTOR_LENGTH + 2;
        Bytes bytes = new Bytes(recordLength);
        for (int i = 0; i < recordLength; i++)
        {
            int result = 0;
            while ((result & 0x80) == 0)
            {
                Bits b = readRawBits(1);
                if (b.size() == 0)
                    break;
                result = (result << 1) | (b.getBit(0) ? 1 : 0);
            }
            bytes.setByte(i, (byte) result);
        }

        /* Upgrade the sector from MISSING to BAD_CHECKSUM. If
         * decodeCrazyData succeeds, it upgrades the sector to OK. */

        sector.status = Sector.Status.BAD_CHECKSUM;
        Sector.Status[] status = {sector.status};
        sector.data = decodeCrazyData(bytes, status);
        sector.status = status[0];
    }
}