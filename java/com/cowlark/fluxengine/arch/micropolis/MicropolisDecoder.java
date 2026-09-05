package com.cowlark.fluxengine.arch.micropolis;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.FmMfm;
import com.cowlark.fluxengine.micropolis.MicropolisDecoderProto;

/**
 * The Micropolis decoder, ported from arch/micropolis/decoder.cc.
 */
public class MicropolisDecoder extends Decoder
{
    /* The sector has a preamble of MFM 0x00s and uses 0xFF as a sync pattern.
     *
     * 00        00        00        F         F
     * 0000 0000 0000 0000 0000 0000 0101 0101 0101 0101
     * A    A    A    A    A    A    5    5    5    5
     */
    private static final FluxPattern SECTOR_SYNC_PATTERN = new FluxPattern(64, 0xAAAAAAAAAAAA5555L);

    /* Pattern to skip past current SYNC. */
    private static final FluxPattern SECTOR_ADVANCE_PATTERN =
            new FluxPattern(64, 0xAAAAAAAAAAAAAAAAL);
    private final MicropolisDecoderProto config;
    private MicropolisDecoderProto.ChecksumType checksumType;

    public MicropolisDecoder(DecoderProto config)
    {
        super(config);
        this.config = config.getMicropolis();
        checksumType = this.config.getChecksumType();
    }

    /* Standard Micropolis checksum. Adds all bytes, with carry. */
    public static int micropolisChecksum(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int sum = 0;
        while (!br.eof())
        {
            if (sum > 0xFF)
            {
                sum -= 0x100 - 1;
            }
            sum += br.read8();
        }
        /* The last carry is ignored. */
        return sum & 0xFF;
    }

    /* Vector MZOS does not use the standard Micropolis checksum. */
    public static int mzosChecksum(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int checksum = 0;

        while (!br.eof())
        {
            int databyte = br.read8();
            checksum ^= ((databyte << 1) | (databyte >>> 7)) & 0xff;
        }

        return checksum;
    }

    private static int b(int field, int pos)
    {
        return (field >>> pos) & 1;
    }

    private static int eccNextBit(int ecc, int dataBit)
    {
        /* This is 0x81932080 which is 0x0104C981 with reversed bits. */
        return b(ecc, 7) ^ b(ecc, 13) ^ b(ecc, 16) ^ b(ecc, 17) ^ b(ecc, 20) ^ b(ecc, 23) ^
                b(ecc, 24) ^ b(ecc, 31) ^ dataBit;
    }

    public static int vectorGraphicEcc(Bytes bytes)
    {
        int e = 0;
        Bytes payloadBytes = bytes.slice(0, bytes.size() - 4);
        ByteReader payload = new ByteReader(payloadBytes);
        while (!payload.eof())
        {
            int byte0 = payload.read8();
            for (int i = 0; i < 8; i++)
            {
                e = (e << 1) | eccNextBit(e, byte0 >>> 7);
                byte0 <<= 1;
            }
        }
        Bytes trailerBytes = bytes.slice(bytes.size() - 4);
        ByteReader trailer = new ByteReader(trailerBytes);
        int res = e;
        while (!trailer.eof())
        {
            int byte0 = trailer.read8();
            for (int i = 0; i < 8; i++)
            {
                res = (res << 1) | eccNextBit(e, byte0 >>> 7);
                e <<= 1;
                byte0 <<= 1;
            }
        }
        return res;
    }

    /* Fixes bytes when possible, returning true if changed. */
    private static boolean vectorGraphicEccFix(Bytes bytes, int syndrome)
    {
        int ecc = syndrome;
        int pos = (Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE - 5) * 8 + 7;
        boolean aligned = false;
        while ((ecc & 0xff000000) == 0)
        {
            pos += 8;
            ecc <<= 8;
        }
        for (; pos >= 0; pos--)
        {
            boolean bit = (ecc & 1) != 0;
            ecc >>>= 1;
            if (bit)
                ecc ^= 0x808264c0;
            if ((ecc & 0xff07ffff) == 0)
                aligned = true;
            if (aligned && pos % 8 == 0)
                break;
        }
        if (pos < 0)
            return false;
        bytes.setByte(pos / 8, (byte) (bytes.getByte(pos / 8) ^ (ecc >>> 16)));
        return true;
    }

    @Override
    protected double advanceToNextRecord()
    {
        double now = tell().getDurationNs();

        /* For all but the first sector, seek to the next sector pulse. The
         * first sector does not contain the sector pulse in the fluxmap. */
        if (now != 0)
        {
            seekToIndexMark();
            now = tell().getDurationNs();
        }

        /* Discard a possible partial sector at the end of the track. */
        if (now > (getFluxmapDuration() - 12.0e6))
        {
            seekToIndexMark();
            return 0;
        }

        double clock = seekToPattern(SECTOR_SYNC_PATTERN);

        double syncDelta = tell().getDurationNs() - now;
        /* Due to the weak nature of the Micropolis SYNC pattern, it's possible
         * to detect a false SYNC during the gap between the sector pulse and
         * the write gate. */
        if ((syncDelta > 0) && (syncDelta < 100e3))
        {
            seekToPattern(SECTOR_ADVANCE_PATTERN);
            clock = seekToPattern(SECTOR_SYNC_PATTERN);
        }

        sector.headerStartTimeNs = tell().getDurationNs();

        /* seekToPattern() can skip past the index hole, if this happens too
         * close to the end of the Fluxmap, discard the sector. */
        if (sector.headerStartTimeNs > (getFluxmapDuration() - 11.3e6))
        {
            return 0;
        }

        return clock;
    }

    @Override
    protected void decodeSectorRecord()
    {
        readRawBits(48);
        com.cowlark.fluxengine.core.Bits rawbits =
                readRawBits(Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE * 16);
        Bytes bytes =
                FmMfm.decodeFmMfm(rawbits).slice(0, Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE);

        boolean eccPresent = (bytes.getByte(274) & 0xff) == 0xaa;
        int ecc = 0;
        if (config.getEccType() == MicropolisDecoderProto.EccType.VECTOR && eccPresent)
        {
            ecc = vectorGraphicEcc(bytes.slice(0, 274));
            if (ecc != 0)
            {
                vectorGraphicEccFix(bytes, ecc);
                ecc = vectorGraphicEcc(bytes.slice(0, 274));
            }
        }

        ByteReader br = bytes.iterator();

        int syncByte = br.read8(); /* sync */
        if (syncByte != 0xFF)
            return;

        int logicalCylinder = br.read8();
        int logicalHead = ltl.logicalHead;
        int logicalSector = br.read8();
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);
        if (logicalSector > 15)
            return;
        if (logicalCylinder > 76)
            return;
        if (logicalCylinder != ltl.logicalCylinder)
            return;

        br.read(10); /* OS data or padding */
        Bytes data = br.read(Micropolis.MICROPOLIS_PAYLOAD_SIZE);
        int wantChecksum = br.read8();

        /* If not specified, automatically determine the checksum type. */
        if (checksumType == MicropolisDecoderProto.ChecksumType.AUTO)
        {
            /* Calculate both standard Micropolis (MDOS, CP/M, OASIS) and MZOS
             * checksums. */
            if (wantChecksum == micropolisChecksum(bytes.slice(1, 2 + 266)))
            {
                checksumType = MicropolisDecoderProto.ChecksumType.MICROPOLIS;
            } else if (wantChecksum == mzosChecksum(bytes.slice(
                    Micropolis.MICROPOLIS_HEADER_SIZE,
                    Micropolis.MICROPOLIS_PAYLOAD_SIZE)))
            {
                checksumType = MicropolisDecoderProto.ChecksumType.MZOS;
                System.out.println("Note: MZOS checksum detected.");
            }
        }

        int gotChecksum;

        if (checksumType == MicropolisDecoderProto.ChecksumType.MZOS)
        {
            gotChecksum = mzosChecksum(bytes.slice(
                    Micropolis.MICROPOLIS_HEADER_SIZE,
                    Micropolis.MICROPOLIS_PAYLOAD_SIZE));
        } else
        {
            gotChecksum = micropolisChecksum(bytes.slice(1, 2 + 266));
        }

        br.read(5); /* 4 byte ECC and ECC-present flag */

        if (config.getSectorOutputSize() == Micropolis.MICROPOLIS_PAYLOAD_SIZE)
            sector.data = data;
        else if (config.getSectorOutputSize() == Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE)
            sector.data = bytes;
        else
            throw new FluxEngineException("Sector output size may only be 256 or 275");
        if (wantChecksum == gotChecksum && (!eccPresent || ecc == 0))
            sector.status = Sector.Status.OK;
        else
            sector.status = Sector.Status.BAD_CHECKSUM;
    }
}