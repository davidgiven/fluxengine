package com.cowlark.fluxengine.arch.northstar;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.FmMfm;
import com.cowlark.fluxengine.northstar.NorthstarEncoderProto;
import java.util.List;

/**
 * The North Star encoder, ported from arch/northstar/encoder.cc.
 */
public class NorthstarEncoder extends Encoder
{
    private static final int GAP_FILL_SIZE_SD = 30;
    private static final int PRE_HEADER_GAP_FILL_SIZE_SD = 9;
    private static final int GAP_FILL_SIZE_DD = 62;
    private static final int PRE_HEADER_GAP_FILL_SIZE_DD = 16;

    private static final int GAP1_FILL_BYTE = 0x4F;
    private static final int GAP2_FILL_BYTE = 0x4F;

    private final NorthstarEncoderProto config;

    public NorthstarEncoder(ConfigProto config, double diskRotationalPeriodNs)
    {
        super(diskRotationalPeriodNs);
        this.config = config.getEncoder().getNorthstar();
    }

    private void writeSector(Bits bits, Bits.Cursor cursor, Sector sector)
    {
        int preambleSize = 0;
        int encodedSectorSize = 0;
        int gapFillSize = 0;
        int preHeaderGapFillSize = 0;

        boolean doubleDensity;

        switch (sector.data.size())
        {
            case Northstar.NORTHSTAR_PAYLOAD_SIZE_SD:
                preambleSize = Northstar.NORTHSTAR_PREAMBLE_SIZE_SD;
                encodedSectorSize =
                        PRE_HEADER_GAP_FILL_SIZE_SD + Northstar.NORTHSTAR_ENCODED_SECTOR_SIZE_SD +
                                GAP_FILL_SIZE_SD;
                gapFillSize = GAP_FILL_SIZE_SD;
                preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_SD;
                doubleDensity = false;
                break;
            case Northstar.NORTHSTAR_PAYLOAD_SIZE_DD:
                preambleSize = Northstar.NORTHSTAR_PREAMBLE_SIZE_DD;
                encodedSectorSize =
                        PRE_HEADER_GAP_FILL_SIZE_DD + Northstar.NORTHSTAR_ENCODED_SECTOR_SIZE_DD +
                                GAP_FILL_SIZE_DD;
                gapFillSize = GAP_FILL_SIZE_DD;
                preHeaderGapFillSize = PRE_HEADER_GAP_FILL_SIZE_DD;
                doubleDensity = true;
                break;
            default:
                throw new FluxEngineException(
                        "unsupported sector size --- you must pick 256 or " + "512");
        }

        int fullSectorSize = preambleSize + encodedSectorSize;
        Bytes fullSector = new Bytes(0);
        ByteWriter fw = fullSector.writer();

        /* sector gap after index pulse */
        for (int i = 0; i < preHeaderGapFillSize; i++)
            fw.write8(GAP1_FILL_BYTE);

        /* sector preamble */
        for (int i = 0; i < preambleSize; i++)
            fw.write8(0);

        Bytes sectorData;
        if (sector.data.size() == encodedSectorSize)
            sectorData = sector.data;
        else
        {
            sectorData = new Bytes(0);
            ByteWriter writer = sectorData.writer();
            writer.write8(0xFB); /* sync character */
            if (doubleDensity)
            {
                writer.write8(0xFB); /* Double-density has two sync characters */
            }
            writer.write(sector.data);
            if (doubleDensity)
            {
                writer.write8(NorthstarDecoder.northstarChecksum(sectorData.slice(2)));
            } else
            {
                writer.write8(NorthstarDecoder.northstarChecksum(sectorData.slice(1)));
            }
        }

        fw.write(sectorData);

        /* sector postamble */
        for (int i = 0; i < gapFillSize; i++)
            fw.write8(GAP2_FILL_BYTE);

        if (sector.location.logicalSector() != 9)
        {
            if (fullSector.size() != fullSectorSize)
                throw new FluxEngineException(String.format(
                        "sector mismatched length (%d); expected %d, got %d",
                        sector.data.size(),
                        fullSector.size(),
                        fullSectorSize));
        }

        boolean[] lastBit = {false};

        if (doubleDensity)
        {
            FmMfm.encodeMfm(bits, cursor, fullSector, lastBit);
        } else
        {
            FmMfm.encodeFm(bits, cursor, fullSector);
        }
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        int bitsPerRevolution = 100000;
        double clockRateUs = config.getClockPeriodUs();

        Sector sector = sectors.get(0);
        if (sector.data.size() == Northstar.NORTHSTAR_PAYLOAD_SIZE_SD)
            bitsPerRevolution /= 2; /* FM */
        else
            clockRateUs /= 2.00;

        Bits bits = new Bits(bitsPerRevolution);
        Bits.Cursor cursor = new Bits.Cursor(0);

        for (Sector sectorData : sectors)
            writeSector(bits, cursor, sectorData);

        if (cursor.get() > bits.size())
            throw new FluxEngineException("track data overrun");

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(bits,
                (long) calculatePhysicalClockPeriodNs(clockRateUs * 1e3,
                        config.getRotationalPeriodMs() * 1e6));
        return fluxmap;
    }
}
