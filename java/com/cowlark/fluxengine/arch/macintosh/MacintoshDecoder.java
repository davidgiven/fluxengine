package com.cowlark.fluxengine.arch.macintosh;

import static com.cowlark.fluxengine.arch.macintosh.Macintosh.MAC_DATA_RECORD;
import static com.cowlark.fluxengine.arch.macintosh.Macintosh.MAC_ENCODED_SECTOR_LENGTH;
import static com.cowlark.fluxengine.arch.macintosh.Macintosh.MAC_SECTOR_LENGTH;
import static com.cowlark.fluxengine.arch.macintosh.Macintosh.MAC_SECTOR_RECORD;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;

/**
 * The Macintosh decoder, ported from arch/macintosh/decoder.cc.
 */
public class MacintoshDecoder extends Decoder
{
    private static final FluxPattern SECTOR_RECORD_PATTERN = new FluxPattern(24, MAC_SECTOR_RECORD);
    private static final FluxPattern DATA_RECORD_PATTERN = new FluxPattern(24, MAC_DATA_RECORD);
    private static final FluxMatchers ANY_RECORD_PATTERN =
            FluxMatchers.of(SECTOR_RECORD_PATTERN, DATA_RECORD_PATTERN);

    public MacintoshDecoder(DecoderProto config)
    {
        super(config);
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

    /* This is extremely inspired by the MESS implementation, written by Nathan
     * Woods and R. Belmont:
     * https://github.com/mamedev/mame/blob/4263a71e64377db11392c458b580c5ae83556bc7/src/lib
     * /formats/ap_dsk35.cpp
     */
    private static Bytes decodeCrazyData(Bytes input, Sector.Status[] status)
    {
        Bytes output = new Bytes();
        ByteWriter bw = new ByteWriter(output);
        ByteReader br = input.iterator();

        int lookupLen = MAC_SECTOR_LENGTH / 3;

        int[] b1 = new int[lookupLen + 1];
        int[] b2 = new int[lookupLen + 1];
        int[] b3 = new int[lookupLen + 1];

        for (int i = 0; i <= lookupLen; i++)
        {
            int w4 = br.read8();
            int w1 = br.read8();
            int w2 = br.read8();
            int w3 = (i != 174) ? br.read8() : 0;

            b1[i] = (w1 & 0x3F) | ((w4 << 2) & 0xC0);
            b2[i] = (w2 & 0x3F) | ((w4 << 4) & 0xC0);
            b3[i] = (w3 & 0x3F) | ((w4 << 6) & 0xC0);
        }

        /* Copy from the user's buffer to our buffer, while computing the
         * three-byte data checksum. */

        int c1 = 0;
        int c2 = 0;
        int c3 = 0;
        int count = 0;
        for (; ; )
        {
            c1 = (c1 & 0xFF) << 1;
            if ((c1 & 0x0100) != 0)
                c1++;

            int val = (b1[count] ^ c1) & 0xFF;
            c3 += val;
            if ((c1 & 0x0100) != 0)
            {
                c3++;
                c1 &= 0xFF;
            }
            bw.write8(val);

            val = (b2[count] ^ c3) & 0xFF;
            c2 += val;
            if (c3 > 0xFF)
            {
                c2++;
                c3 &= 0xFF;
            }
            bw.write8(val);

            if (output.size() == 524)
                break;

            val = (b3[count] ^ c2) & 0xFF;
            c1 += val;
            if (c2 > 0xFF)
            {
                c1++;
                c2 &= 0xFF;
            }
            bw.write8(val);
            count++;
        }

        int c4 = ((c1 & 0xC0) >> 6) | ((c2 & 0xC0) >> 4) | ((c3 & 0xC0) >> 2);
        c1 &= 0x3f;
        c2 &= 0x3f;
        c3 &= 0x3f;
        c4 &= 0x3f;
        int g4 = br.read8();
        int g3 = br.read8();
        int g2 = br.read8();
        int g1 = br.read8();
        if ((g4 == c4) && (g3 == c3) && (g2 == c2) && (g1 == c1))
            status[0] = Sector.Status.OK;

        return output;
    }

    private static int decodeSide(int side)
    {
        /* Mac disks, being weird, use the side byte to encode both the side
         * (in bit 5) and also whether we're above track 0x3f (in bit 0). */

        return (side & 0x20) != 0 ? 1 : 0;
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(ANY_RECORD_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        if (readRaw24() != MAC_SECTOR_RECORD)
            return;

        /* Read header. */

        Bytes header = readRawBits(7 * 8).toBytes().slice(0, 7);

        int encodedTrack = decodeDataGcr(header.getByte(0));
        if (encodedTrack != (ltl.logicalCylinder & 0x3f))
            return;

        int encodedSector = decodeDataGcr(header.getByte(1));
        int encodedSide = decodeDataGcr(header.getByte(2));
        int formatByte = decodeDataGcr(header.getByte(3));
        int wantedsum = decodeDataGcr(header.getByte(4));

        if (encodedSector > 11)
            return;

        int logicalCylinder = ltl.logicalCylinder;
        int logicalHead = decodeSide(encodedSide);
        int logicalSector = encodedSector;
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);
        int gotsum = (encodedTrack ^ encodedSector ^ encodedSide ^ formatByte) & 0x3f;
        if (wantedsum == gotsum)
            sector.status = Sector.Status.DATA_MISSING;
    }

    @Override
    protected void decodeDataRecord()
    {
        if (readRaw24() != MAC_DATA_RECORD)
            return;

        /* Read data. */

        readRawBits(8); /* skip spare byte */
        Bytes inputbuffer = readRawBits(MAC_ENCODED_SECTOR_LENGTH * 8)
                .toBytes()
                .slice(0, MAC_ENCODED_SECTOR_LENGTH);

        for (int i = 0; i < inputbuffer.size(); i++)
            inputbuffer.setByte(i, decodeDataGcr(inputbuffer.getByte(i)));

        Sector.Status[] status = {Sector.Status.BAD_CHECKSUM};
        sector.status = status[0];
        Bytes userData = decodeCrazyData(inputbuffer, status);
        sector.status = status[0];
        sector.data = new Bytes();
        sector.data.writer().write(userData.slice(12, 512)).write(userData.slice(0, 12));
    }
}