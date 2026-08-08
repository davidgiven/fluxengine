package com.cowlark.fluxengine.decoders;

import static com.cowlark.fluxengine.external.FluxEngine.F_BIT_PULSE;

import com.cowlark.fluxengine.core.Bits;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.FluxMatcher;
import com.cowlark.fluxengine.data.FluxPosition;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.data.FluxmapReader;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.PhysicalTrackLayout;
import com.cowlark.fluxengine.data.Record;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;

/**
 * The base class for track decoders, ported from lib/decoders/decoders.{h,cc}.
 */
public abstract class Decoder
{
    public enum RecordType
    {
        SECTOR_RECORD, DATA_RECORD, UNKNOWN_RECORD
    }

    protected final DecoderProto config;
    protected LogicalTrackLayout ltl;
    protected Track trackdata;
    protected Sector sector;
    protected FluxDecoder decoder;
    protected Bits recordBits = new Bits();

    private FluxmapReader fmr;

    public Decoder(DecoderProto config)
    {
        this.config = config;
    }

    public Track decodeToSectors(Fluxmap fluxmap, PhysicalTrackLayout ptl)
    {
        ltl = ptl.logicalTrackLayout;

        trackdata = new Track();
        trackdata.fluxmap = fluxmap;
        trackdata.ptl = ptl;
        trackdata.ltl = ptl.logicalTrackLayout;

        FluxmapReader fmrLocal = new FluxmapReader(fluxmap, config);
        fmr = fmrLocal;

        newSector();
        beginTrack();
        for (; ; )
        {
            newSector();

            FluxPosition recordStart = fmr.tell();
            sector.clockNs = advanceToNextRecord();
            if (fmr.eof() || sector.clockNs == 0)
                break;

            /* Read the sector record. */

            FluxPosition before = fmr.tell();
            decodeSectorRecord();
            FluxPosition after = fmr.tell();
            pushRecord(before, after);

            if (sector.status != Sector.Status.DATA_MISSING)
            {
                sector.position = before.bytes();
                sector.dataStartTimeNs = before.getDurationNs();
                sector.dataEndTimeNs = after.getDurationNs();
            } else
            {
                /* The data is in a separate record. */

                sector.headerStartTimeNs = before.getDurationNs();
                sector.headerEndTimeNs = after.getDurationNs();

                sector.clockNs = advanceToNextRecord();
                if (fmr.eof() || sector.clockNs == 0)
                    break;

                before = fmr.tell();
                decodeDataRecord();
                sector.data = sector.data.slice(0, ltl.sectorSize);
                after = fmr.tell();

                if (sector.status != Sector.Status.DATA_MISSING)
                {
                    sector.position = before.bytes();
                    sector.dataStartTimeNs = before.getDurationNs();
                    sector.dataEndTimeNs = after.getDurationNs();
                    pushRecord(before, after);
                } else
                {
                    fmr.skipToEvent(F_BIT_PULSE);
                    resetFluxDecoder();
                }
            }

            if (sector.status != Sector.Status.MISSING)
                trackdata.allSectors.add(sector);
        }

        return trackdata;
    }

    private void newSector()
    {
        sector = new Sector(new LogicalLocation(0, 0, 0));
        sector.physicalLocation =
                new CylinderHead(trackdata.ptl.physicalCylinder, trackdata.ptl.physicalHead);
        sector.status = Sector.Status.MISSING;
    }

    protected void pushRecord(FluxPosition start, FluxPosition end)
    {
        Record record = new Record();
        trackdata.records.add(record);
        sector.records.add(record);

        record.position = start.bytes();
        record.startTimeNs = start.getDurationNs();
        record.endTimeNs = end.getDurationNs();
        record.clockNs = sector.clockNs;

        record.rawData = recordBits.toBytes();
        recordBits = new Bits();
    }

    protected void resetFluxDecoder()
    {
        decoder = new FluxDecoder(fmr, sector.clockNs, config);
    }

    public double seekToPattern(FluxMatcher pattern)
    {
        double clockNs = fmr.seekToPattern(pattern);
        decoder = new FluxDecoder(fmr, clockNs, config);
        return clockNs;
    }

    public void seekToIndexMark()
    {
        fmr.skipToEvent(F_BIT_PULSE);
        fmr.seekToIndexMark();
    }

    public Bits readRawBits(int count)
    {
        Bits bits = decoder.readBits(count);
        for (int i = 0; i < bits.size(); i++)
            recordBits.add(bits.getBit(i));
        return bits;
    }

    public int readRaw8()
    {
        return readRawBits(8).toBytes().iterator().read8();
    }

    public int readRaw16()
    {
        return readRawBits(16).toBytes().iterator().readBe16();
    }

    public int readRaw20()
    {
        Bits bits = new Bits();
        for (int i = 0; i < 4; i++)
            bits.add(false);
        Bits raw = readRawBits(20);
        for (int i = 0; i < raw.size(); i++)
            bits.add(raw.getBit(i));
        return bits.toBytes().iterator().readBe24();
    }

    public int readRaw24()
    {
        return readRawBits(24).toBytes().iterator().readBe24();
    }

    public int readRaw32()
    {
        return readRawBits(32).toBytes().iterator().readBe32();
    }

    public long readRaw48()
    {
        return readRawBits(48).toBytes().iterator().readBe48();
    }

    public long readRaw64()
    {
        return readRawBits(64).toBytes().iterator().readBe64();
    }

    public FluxPosition tell()
    {
        return fmr.tell();
    }

    public void rewind()
    {
        fmr.rewind();
    }

    public void seek(FluxPosition pos)
    {
        fmr.seek(pos);
    }

    public boolean eof()
    {
        return fmr.eof();
    }

    public double getFluxmapDuration()
    {
        return fmr.getDurationNs();
    }

    protected void beginTrack()
    {
    }

    protected abstract double advanceToNextRecord();

    protected abstract void decodeSectorRecord();

    protected void decodeDataRecord()
    {
    }
}