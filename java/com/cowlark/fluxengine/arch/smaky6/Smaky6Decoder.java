package com.cowlark.fluxengine.arch.smaky6;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.FluxPattern;
import com.cowlark.fluxengine.data.FluxPosition;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.external.Crc;
import com.cowlark.fluxengine.external.FmMfm;
import java.util.ArrayList;
import java.util.List;

/**
 * The Smaky6 decoder, ported from arch/smaky6/decoder.cc.
 */
public class Smaky6Decoder extends Decoder
{
    private static final FluxPattern SECTOR_PATTERN = new FluxPattern(32, 0x54892aaa);
    private final List<SectorStart> sectorStarts = new ArrayList<>();
    private int sectorId;
    private int sectorIndex;

    public Smaky6Decoder(DecoderProto config)
    {
        super(config);
    }

    /* Returns the sector ID of the _current_ sector. */
    private int advanceToNextSector()
    {
        FluxPosition previous = tell();
        seekToIndexMark();
        FluxPosition now = tell();
        if ((now.getDurationNs() - previous.getDurationNs()) < 9e6)
        {
            seekToIndexMark();
            FluxPosition next = tell();
            if ((next.getDurationNs() - now.getDurationNs()) < 9e6)
            {
                /* We just found sector 0. */

                sectorId = 0;
            } else
            {
                /* Spurious... */

                seek(now);
            }
        }

        return sectorId++;
    }

    @Override
    protected void beginTrack()
    {
        /* Find the start-of-track index marks, which will be an interval of
         * about 6ms. */

        seekToIndexMark();
        sectorId = 99;
        for (; ; )
        {
            FluxPosition pos = tell();
            advanceToNextSector();
            if (sectorId < 99)
            {
                seek(pos);
                break;
            }

            if (eof())
                return;
        }

        /* Now we know where to start counting, start finding sectors. */

        sectorStarts.clear();
        for (; ; )
        {
            FluxPosition now = tell();
            if (eof())
                break;

            int id = advanceToNextSector();
            if (id < 16)
                sectorStarts.add(new SectorStart(id, now));
        }

        sectorIndex = 0;
    }

    @Override
    protected double advanceToNextRecord()
    {
        if (sectorIndex == sectorStarts.size())
        {
            seekToIndexMark();
            return 0;
        }

        SectorStart p = sectorStarts.get(sectorIndex++);
        sectorId = p.id();
        seek(p.pos());

        double clock = seekToPattern(SECTOR_PATTERN);
        sector.headerStartTimeNs = tell().getDurationNs();

        return clock;
    }

    @Override
    protected void decodeSectorRecord()
    {
        readRawBits(33);
        Bits rawbits = readRawBits(Smaky6.SMAKY6_RECORD_SIZE * 16);
        if (rawbits.size() < Smaky6.SMAKY6_SECTOR_SIZE)
            return;

        /* The Smaky bytes are stored backwards! Backwards! */

        Bytes bytes = FmMfm.decodeFmMfm(rawbits).slice(0, Smaky6.SMAKY6_RECORD_SIZE).reverseBits();
        ByteReader br = bytes.iterator();

        int track = br.read8();
        Bytes data = br.read(Smaky6.SMAKY6_SECTOR_SIZE);
        int wantedChecksum = br.read8();
        int gotChecksum = Crc.sumBytes(data) & 0xff;

        if (track != ltl.logicalCylinder)
            return;

        int logicalCylinder = ltl.physicalCylinder;
        int logicalHead = ltl.logicalHead;
        int logicalSector = sectorId;
        sector.location = new LogicalLocation(logicalCylinder, logicalHead, logicalSector);

        sector.data = data;
        sector.status =
                (wantedChecksum == gotChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
    }

    private record SectorStart(int id, FluxPosition pos)
    {
    }
}