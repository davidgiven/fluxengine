package com.cowlark.fluxengine.arch.mx;

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
 * The MX decoder, ported from arch/mx/decoder.cc.
 */
public class MxDecoder extends Decoder
{
    /*
     * MX disks are a bunch of sectors glued together with no gaps or sync
     * markers, following a single beginning-of-track synchronisation and
     * identification sequence.
     */

    /* FM beginning of track marker:
     *         0         0         f         3 decoded nibbles
     *  0 0  0 0  0 0  0 0  1 1  1 1  0 0  1 1
     * 1010 1010 1010 1010 1111 1111 1010 1111
     *    a    a    a    a    f    f    a    f encoded nibbles
     */
    private static final FluxPattern ID_PATTERN = new FluxPattern(32, 0xaaaaffaf);

    private double clock;
    private int currentSector;

    public MxDecoder(DecoderProto config)
    {
        super(config);
    }

    @Override
    protected void beginTrack()
    {
        clock = sector.clockNs = seekToPattern(ID_PATTERN);
        currentSector = 0;
    }

    @Override
    protected double advanceToNextRecord()
    {
        if (currentSector == 11)
        {
            /* That was the last sector on the disk. */
            return 0;
        } else
        {
            return clock;
        }
    }

    @Override
    protected void decodeSectorRecord()
    {
        /* Skip the ID pattern and track word, which is only present on the
         * first sector. We don't trust the track word because some drivers
         * don't write it correctly. */

        if (currentSector == 0)
            readRawBits(64);

        Bits bits = readRawBits((Mx.SECTOR_SIZE + 2) * 16);
        Bytes bytes = FmMfm.decodeFmMfm(bits).slice(0, Mx.SECTOR_SIZE + 2);

        int gotChecksum = 0;
        ByteReader br = bytes.iterator();
        for (int i = 0; i < (Mx.SECTOR_SIZE / 2); i++)
            gotChecksum += br.readBe16();
        int wantChecksum = br.readBe16();

        int logicalCylinder = ltl.logicalCylinder;
        int logicalHead = ltl.logicalHead;
        int logicalSector = currentSector;
        sector.logicalLocation =
                new CylinderHeadSector(logicalCylinder, logicalHead, logicalSector);
        sector.data = bytes.slice(0, Mx.SECTOR_SIZE).swab();
        sector.status =
                (gotChecksum == wantChecksum) ? Sector.Status.OK : Sector.Status.BAD_CHECKSUM;
        currentSector++;
    }
}