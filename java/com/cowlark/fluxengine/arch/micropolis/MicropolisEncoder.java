package com.cowlark.fluxengine.arch.micropolis;

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
import com.cowlark.fluxengine.micropolis.MicropolisEncoderProto;
import java.util.ArrayList;
import java.util.List;

/**
 * The Micropolis encoder, ported from arch/micropolis/encoder.cc.
 */
public class MicropolisEncoder extends Encoder
{
    private final ConfigProto fullConfig;
    private final MicropolisEncoderProto config;

    public MicropolisEncoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getMicropolis();
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        int bitsPerRevolution =
                (int) ((config.getRotationalPeriodMs() * 1e3) / config.getClockPeriodUs());

        Bits bits = new Bits(bitsPerRevolution);
        List<Integer> indexes = new ArrayList<>();
        int prevCursor = 0;
        Bits.Cursor cursor = new Bits.Cursor(0);

        for (Sector sectorData : sectors)
        {
            indexes.add(cursor.get());
            prevCursor = cursor.get();
            writeSector(bits, cursor, sectorData, config.getEccType());
        }
        indexes.add(prevCursor + (cursor.get() - prevCursor) / 2);
        indexes.add(cursor.get());

        if (cursor.get() != bits.size())
            throw new FluxEngineException("track data mismatched length");

        Fluxmap fluxmap = new Fluxmap();
        long clockPeriod =
                (long) calculatePhysicalClockPeriod(
                        fullConfig,
                        config.getClockPeriodUs() * 1e3,
                        config.getRotationalPeriodMs() * 1e6);
        int pos = 0;
        for (int i = 1; i < indexes.size(); i++)
        {
            int end = indexes.get(i);
            fluxmap.appendBits(bits.subList(pos, end), clockPeriod);
            fluxmap.appendIndex();
            pos = end;
        }
        return fluxmap;
    }

    private void writeSector(Bits bits,
            Bits.Cursor cursor,
            Sector sector,
            MicropolisEncoderProto.EccType eccType)
    {
        if ((sector.data.size() != 256) && (sector.data.size() != Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE))
            throw new FluxEngineException("unsupported sector size --- you must pick 256 or 275");

        int fullSectorSize = 40 + Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE + 40 + 35;
        Bytes fullSector = new Bytes(0);
        ByteWriter fullSectorWriter = fullSector.writer();

        /* sector preamble */
        for (int i = 0; i < 40; i++)
            fullSectorWriter.write8(0);

        Bytes sectorData;
        if (sector.data.size() == Micropolis.MICROPOLIS_ENCODED_SECTOR_SIZE)
        {
            if ((sector.data.getByte(0) & 0xff) != 0xFF)
                throw new FluxEngineException(
                        "275 byte sector doesn't start with sync byte 0xFF. Corrupted sector");
            int wantChecksum = sector.data.getByte(1 + 2 + 266) & 0xff;
            int gotChecksum = MicropolisDecoder.micropolisChecksum(sector.data.slice(1, 2 + 266));
            if (wantChecksum != gotChecksum)
                System.err.println(
                        "Warning: checksum incorrect. Sector: " + sector.location.logicalSector());
            sectorData = sector.data;
        }
        else
        {
            sectorData = new Bytes(0);
            ByteWriter writer = sectorData.writer();
            writer.write8(0xff); /* Sync */
            writer.write8(sector.location.logicalCylinder());
            writer.write8(sector.location.logicalSector());
            for (int i = 0; i < 10; i++)
                writer.write8(0); /* Padding */
            writer.write(sector.data);
            writer.write8(MicropolisDecoder.micropolisChecksum(sectorData.slice(1)));

            int eccPresent = 0;
            int ecc = 0;
            if (eccType == MicropolisEncoderProto.EccType.VECTOR)
            {
                eccPresent = 0xaa;
                ecc = MicropolisDecoder.vectorGraphicEcc(sectorData.concat(new Bytes(4)));
            }
            writer.writeBe32(ecc);
            writer.write8(eccPresent);
        }

        fullSectorWriter.write(sectorData);

        /* sector postamble */
        for (int i = 0; i < 40; i++)
            fullSectorWriter.write8(0);
        /* filler */
        for (int i = 0; i < 35; i++)
            fullSectorWriter.write8(0);

        if (fullSector.size() != fullSectorSize)
            throw new FluxEngineException("sector mismatched length");

        boolean[] lastBit = {false};
        FmMfm.encodeMfm(bits, cursor, fullSector, lastBit);
        /* filler */
        for (int i = 0; i < 5; i++)
        {
            bits.setBit(cursor.get(), true);
            cursor.advance();
            bits.setBit(cursor.get(), false);
            cursor.advance();
        }
    }
}
