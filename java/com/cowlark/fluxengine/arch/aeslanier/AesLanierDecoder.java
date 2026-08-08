package com.cowlark.fluxengine.arch.aeslanier;

import static com.cowlark.fluxengine.arch.aeslanier.AesLanier.AESLANIER_RECORD_SEPARATOR;
import static com.cowlark.fluxengine.arch.aeslanier.AesLanier.AESLANIER_RECORD_SIZE;
import static com.cowlark.fluxengine.arch.aeslanier.AesLanier.AESLANIER_SECTOR_LENGTH;
import static com.cowlark.fluxengine.external.Crc.MODBUS_POLY_REF;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * The AES Lanier decoder, ported from arch/aeslanier/decoder.cc.
 */
public class AesLanierDecoder extends Decoder
{
    private static final FluxPattern SECTOR_PATTERN =
            new FluxPattern(32, AESLANIER_RECORD_SEPARATOR);

    public AesLanierDecoder(DecoderProto config)
    {
        super(config);
    }

    /* This is actually M2FM, rather than MFM, but our MFM/FM decoder copes fine
     * with it. */

    @Override
    protected double advanceToNextRecord()
    {
        return seekToPattern(SECTOR_PATTERN);
    }

    @Override
    protected void decodeSectorRecord()
    {
        /* Skip ID mark (we know it's a AESLANIER_RECORD_SEPARATOR). */

        readRawBits(16);

        Bits rawbits = readRawBits(AESLANIER_RECORD_SIZE * 16);
        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, AESLANIER_RECORD_SIZE);
        Bytes reversed = bytes.reverseBits();

        sector.location =
                new LogicalLocation(reversed.getByte(1) & 0xff, 0, reversed.getByte(2) & 0xff);

        /* Check header 'checksum' (which seems far too simple to mean much). */

        {
            int wanted = reversed.getByte(3) & 0xff;
            int got = ((reversed.getByte(1) & 0xff) + (reversed.getByte(2) & 0xff)) & 0xff;
            if (wanted != got)
                return;
        }

        /* Check data checksum, which also includes the header and is
         * significantly better. */

        sector.data = reversed.slice(1, AESLANIER_SECTOR_LENGTH);
        int wanted = reversed.iterator().seek(0x101).readLe16();
        int got = Crc.crc16ref(MODBUS_POLY_REF, sector.data);
        sector.status = (wanted == got) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}