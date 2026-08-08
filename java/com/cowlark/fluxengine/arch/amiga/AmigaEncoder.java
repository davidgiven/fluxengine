package com.cowlark.fluxengine.arch.amiga;

import com.cowlark.fluxengine.amiga.AmigaEncoderProto;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.external.FmMfm;
import java.util.List;

/**
 * The Amiga encoder, ported from arch/amiga/encoder.cc.
 */
public class AmigaEncoder extends Encoder
{
    private final ConfigProto fullConfig;
    private final AmigaEncoderProto config;
    private final boolean[] lastBit = new boolean[1];

    public AmigaEncoder(ConfigProto config)
    {
        this.fullConfig = config;
        this.config = config.getEncoder().getAmiga();
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, boolean[] src)
    {
        for (boolean bit : src)
        {
            if (cursor.get() < bits.size())
            {
                lastBit[0] = bit;
                bits.setBit(cursor.get(), bit);
                cursor.advance();
            }
        }
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, long data, int width)
    {
        cursor.advance(width);
        lastBit[0] = (data & 1) != 0;
        for (int i = 0; i < width; i++)
        {
            int pos = cursor.get() - i - 1;
            if (pos < bits.size())
                bits.setBit(pos, (data & 1) != 0);
            data >>= 1;
        }
    }

    private void writeBits(Bits bits, Bits.Cursor cursor, Bytes bytes)
    {
        Bits bitr = bytes.toBits();
        for (int i = 0; i < bitr.size(); i++)
        {
            if (cursor.get() < bits.size())
            {
                bits.setBit(cursor.get(), bitr.getBit(i));
                cursor.advance();
            }
        }
    }

    private void writeInterleavedBytes(Bits bits, Bits.Cursor cursor, Bytes bytes, int[] checksum)
    {
        Bytes interleaved = Amiga.amigaInterleave(bytes);
        Bytes mfm = FmMfm.encodeMfm(interleaved, lastBit);
        checksum[0] ^= Amiga.amigaChecksum(mfm);
        checksum[0] &= 0x55555555;
        writeBits(bits, cursor, mfm);
    }

    private void writeInterleavedWord(Bits bits, Bits.Cursor cursor, int word, int[] checksum)
    {
        Bytes b = new Bytes(4);
        b.writer().writeBe32(word);
        writeInterleavedBytes(bits, cursor, b, checksum);
    }

    private void writeSector(Bits bits, Bits.Cursor cursor, Sector sector)
    {
        if ((sector.data.size() != 512) && (sector.data.size() != 528))
            throw new FluxEngineException("unsupported sector size --- you must pick 512 or 528");

        int[] checksum = {0};

        writeBits(bits, cursor, 0xaaaa, 2 * 8);
        writeBits(bits, cursor, Amiga.AMIGA_SECTOR_RECORD, 6 * 8);

        Bytes header = Bytes.of(
                0xff, /* Amiga 1.0 format byte */
                (sector.location.logicalCylinder() << 1) | sector.location.logicalHead(),
                sector.location.logicalSector(),
                Amiga.AMIGA_SECTORS_PER_TRACK - sector.location.logicalSector());
        writeInterleavedBytes(bits, cursor, header, checksum);
        Bytes recoveryInfo = new Bytes(16);
        if (sector.data.size() == 528)
            recoveryInfo = sector.data.slice(512, 16);
        writeInterleavedBytes(bits, cursor, recoveryInfo, checksum);
        writeInterleavedWord(bits, cursor, checksum[0], checksum);

        Bytes data = sector.data.slice(0, 512);
        writeInterleavedWord(
                bits,
                cursor,
                Amiga.amigaChecksum(FmMfm.encodeMfm(Amiga.amigaInterleave(data), lastBit)),
                checksum);
        writeInterleavedBytes(bits, cursor, data, checksum);
    }

    @Override
    public Fluxmap encode(LogicalTrackLayout ltl, List<Sector> sectors, Image image)
    {
        /* Number of bits for one nominal revolution of a real 200ms Amiga
         * disk. */
        int bitsPerRevolution = (int) (200e3 / config.getClockRateUs());
        Bits bits = new Bits(bitsPerRevolution);
        Bits.Cursor cursor = new Bits.Cursor(0);

        bits.fillBitmapTo(
                cursor,
                (int) (config.getPostIndexGapMs() * 1000 / config.getClockRateUs()),
                new boolean[]{true, false});
        lastBit[0] = false;

        for (Sector sector : sectors)
            writeSector(bits, cursor, sector);

        if (cursor.get() >= bits.size())
            throw new FluxEngineException("track data overrun");
        bits.fillBitmapTo(cursor, bits.size(), new boolean[]{true, false});

        Fluxmap fluxmap = new Fluxmap();
        fluxmap.appendBits(
                bits,
                (long) calculatePhysicalClockPeriod(
                        fullConfig,
                        config.getClockRateUs() * 1e3,
                        200e6));
        return fluxmap;
    }
}
