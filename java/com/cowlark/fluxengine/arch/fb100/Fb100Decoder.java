package com.cowlark.fluxengine.arch.fb100;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The FB100 decoder, ported from arch/fb100/decoder.cc.
 */
public class Fb100Decoder extends Decoder
{
    private static final FluxPattern SECTOR_ID_PATTERN = new FluxPattern(16, 0xabaa);

    public Fb100Decoder(DecoderProto config)
    {
        super(config);
    }

    /*
     * Reverse engineered from a dump of the floppy drive's ROM. I have no idea
     * how it works.
     *
     * LF8BA:
     *         clra
     *         staa    X00B0
     *         staa    X00B1
     *         ldx     #$8000
     * LF8C2:  ldaa    $00,x
     *         inx
     *         bsr     LF8CF
     *         cpx     #$8011
     *         bne     LF8C2
     *         ldd     X00B0
     *         rts
     * LF8CF:
     *         eora    X00B0
     *         staa    X00CF
     *         asla
     *         asla
     *         asla
     *         asla
     *         eora    X00CF
     *         staa    X00CF
     *         rola
     *         rola
     *         rola
     *         tab
     *         anda    #$F8
     *         eora    X00B1
     *         staa    X00B0
     *         rolb
     *         rolb
     *         andb    #$0F
     *         eorb    X00B0
     *         stab    X00B0
     *         rolb
     *         eorb    X00CF
     *         stab    X00B1
     *         rts
     */
    private static void rol(int[] b, boolean[] c)
    {
        boolean newc = (b[0] & 0x80) != 0;
        b[0] = ((b[0] << 1) | (c[0] ? 1 : 0)) & 0xff;
        c[0] = newc;
    }

    private static int checksum(Bytes bytes)
    {
        int crclo = 0;
        int crchi = 0;
        for (int i = 0; i < bytes.size(); i++)
        {
            int a = bytes.getByte(i) & 0xff;
            a ^= crchi;
            int t1 = a;
            a <<= 4;
            boolean[] c = {((a & 0x10) != 0)};
            a ^= t1;
            t1 = a;
            int[] b = {a};
            rol(b, c);
            rol(b, c);
            rol(b, c);
            a = b[0];
            a &= 0xf8;
            a ^= crclo;
            crchi = a;
            rol(b, c);
            rol(b, c);
            b[0] &= 0x0f;
            b[0] ^= crchi;
            crchi = b[0];
            rol(b, c);
            b[0] ^= t1;
            crclo = b[0];
        }

        return (crchi << 8) | crclo;
    }

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(SECTOR_ID_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        Bits rawbits = readRawBits(Fb100.FB100_RECORD_SIZE * 16);

        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, Fb100.FB100_RECORD_SIZE);
        ByteReader br = bytes.iterator();
        br.seek(1);
        Bytes id = br.read(Fb100.FB100_ID_SIZE);
        int wantIdCrc = br.readBe16();
        int gotIdCrc = checksum(id);
        Bytes payload = br.read(Fb100.FB100_PAYLOAD_SIZE);
        int wantPayloadCrc = br.readBe16();
        int gotPayloadCrc = checksum(payload);

        if (wantIdCrc != gotIdCrc)
            return;

        int abssector = id.getByte(2) & 0xff;
        int logicalCylinder = abssector >> 1;
        int logicalHead = 0;
        int logicalSector = abssector & 1;
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);

        Bytes data = new Bytes();
        data.writer().write(id.slice(5, 12)).write(payload);
        sector.data = data;

        sector.status =
                (wantPayloadCrc == gotPayloadCrc) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}