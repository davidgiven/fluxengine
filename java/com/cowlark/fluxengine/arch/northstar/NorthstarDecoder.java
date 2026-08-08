package com.cowlark.fluxengine.arch.northstar;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxMatchers;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.FmMfm;

/**
 * Decoder for North Star 10-sector hard-sectored disks, ported from
 * arch/northstar/decoder.cc.
 *
 * Supports both single- and double-density. For the sector format and
 * checksum algorithm, see pp. 33 of the North Star Double Density Controller
 * manual:
 *
 * http://bitsavers.org/pdf/northstar/boards/Northstar_MDS-A-D_1978.pdf
 *
 * North Star disks do not contain any track/head/sector information encoded in
 * the sector record. For this reason, we have to be absolutely sure that the
 * hardSectorId is correct.
 */
public class NorthstarDecoder extends Decoder
{
    private static final long MFM_ID = 0xaaaaaaaaaaaa5545L;
    private static final long FM_ID = 0xaaaaaaaaaaaaffefL;

    /*
     * MFM sectors have 32 bytes of 00's followed by two sync characters,
     * specified in the North Star MDS manual as 0xFBFB.
     *
     * This is true for most disks; however, I found a few disks, including an
     * original North Star DOS/BASIC v2.2.1 DQ disk) that uses 0xFBnn, where
     * nn is an incrementing pattern.
     *
     * 00        00        00        F         B
     * 0000 0000 0000 0000 0000 0000 0101 0101 0100 0101
     * A    A    A    A    A    A    5    5    4    5
     */
    private static final FluxPattern MFM_PATTERN = new FluxPattern(64, MFM_ID);

    /* FM sectors have 16 bytes of 00's followed by 0xFB.
     * 00        FB
     * 0000 0000 1111 1111 1110 1111
     * A    A    F    F    E    F
     */
    private static final FluxPattern FM_PATTERN = new FluxPattern(64, FM_ID);

    private static final FluxMatchers ANY_SECTOR_PATTERN = FluxMatchers.of(MFM_PATTERN, FM_PATTERN);

    /* Checksum is initially 0. For each data byte, XOR with the current
     * checksum. Rotate checksum left, carrying bit 7 to bit 0. */
    public static int northstarChecksum(Bytes bytes)
    {
        ByteReader br = new ByteReader(bytes);
        int checksum = 0;

        while (!br.eof())
        {
            checksum ^= br.read8();
            checksum = ((checksum << 1) | (checksum >>> 7)) & 0xff;
        }

        return checksum;
    }

    private int hardSectorId;

    public NorthstarDecoder(DecoderProto config)
    {
        super(config);
    }

    /* Search for FM or MFM sector record. */
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
        if (now > (getFluxmapDuration() - 21e6))
        {
            seekToIndexMark();
            return 0;
        }

        double clock = seekToPattern(ANY_SECTOR_PATTERN);
        sector.headerStartTimeNs = tell().getDurationNs();

        /* Discard a possible partial sector. */
        if (sector.headerStartTimeNs > (getFluxmapDuration() - 21e6))
        {
            return 0;
        }

        double sectorFoundTimeRaw = Math.round(sector.headerStartTimeNs / 1e6);
        double sectorFoundTime;

        /* Round time to the nearest 20ms. */
        if ((sectorFoundTimeRaw % 20) < 10)
        {
            sectorFoundTime = (sectorFoundTimeRaw / 20) * 20;
        } else
        {
            sectorFoundTime = ((sectorFoundTimeRaw + 20) / 20) * 20;
        }

        /* Calculate the sector ID based on time since the index. */
        hardSectorId = (int) ((sectorFoundTime / 20) % 10);

        return clock;
    }

    @Override
    protected void decodeSectorRecord()
    {
        long id = readRawBits(64).toBytes().iterator().readBe64();
        int recordSize;
        int payloadSize;
        int headerSize;

        if (id == MFM_ID)
        {
            recordSize = Northstar.NORTHSTAR_ENCODED_SECTOR_SIZE_DD;
            payloadSize = Northstar.NORTHSTAR_PAYLOAD_SIZE_DD;
            headerSize = Northstar.NORTHSTAR_HEADER_SIZE_DD;
        } else
        {
            recordSize = Northstar.NORTHSTAR_ENCODED_SECTOR_SIZE_SD;
            payloadSize = Northstar.NORTHSTAR_PAYLOAD_SIZE_SD;
            headerSize = Northstar.NORTHSTAR_HEADER_SIZE_SD;
        }

        Bits rawbits = readRawBits(recordSize * 16);
        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, recordSize);
        ByteReader br = bytes.iterator();

        int logicalHead = ltl.logicalHead;
        int logicalSector = hardSectorId;
        int logicalCylinder = ltl.logicalCylinder;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);

        if (headerSize == Northstar.NORTHSTAR_HEADER_SIZE_DD)
        {
            br.read8(); /* MFM second Sync char, usually 0xFB */
        }

        sector.data = br.read(payloadSize);
        int wantChecksum = br.read8();
        int gotChecksum = northstarChecksum(bytes.slice(headerSize - 1, payloadSize));
        sector.status = (wantChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }
}