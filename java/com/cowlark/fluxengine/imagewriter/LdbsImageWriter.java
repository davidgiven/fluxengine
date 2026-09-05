package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.external.Ldbs;

/**
 * Writes an LDBS sector image, ported from
 * lib/imagewriter/ldbsimagewriter.cc.
 */
public class LdbsImageWriter extends ImageWriter
{
    public LdbsImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    @Override
    public void writeImage(Image image)
    {
        Ldbs ldbs = new Ldbs();

        Geometry geometry = image.getGeometry();

        Logger.logf(
                "LDBS: writing %d tracks, %d sides, %d sectors, %d bytes per sector",
                geometry.numCylinders,
                geometry.numHeads,
                geometry.numSectors,
                geometry.sectorSize);

        Bytes trackDirectory = new Bytes();
        ByteWriter trackDirectoryWriter = new ByteWriter(trackDirectory);
        int trackDirectorySize = 0;
        trackDirectoryWriter.writeLe16(0);

        LDBSOutputProto.DataRate dataRate = getWriterConfig().getLdbs().getDataRate();
        if (dataRate == LDBSOutputProto.DataRate.RATE_GUESS)
        {
            dataRate = (geometry.numSectors > 10) ?
                    LDBSOutputProto.DataRate.RATE_HD :
                    LDBSOutputProto.DataRate.RATE_DD;
            if (geometry.sectorSize <= 256)
                dataRate = LDBSOutputProto.DataRate.RATE_SD;
            Logger.logf("LDBS: guessing data rate as %s", dataRate.name());
        }

        LDBSOutputProto.RecordingMode recordingMode =
                getWriterConfig().getLdbs().getRecordingMode();
        if (recordingMode == LDBSOutputProto.RecordingMode.RECMODE_GUESS)
        {
            recordingMode = LDBSOutputProto.RecordingMode.RECMODE_MFM;
            Logger.logf("LDBS: guessing recording mode as %s", recordingMode.name());
        }

        for (int track = 0; track < geometry.numCylinders; track++)
        {
            for (int side = 0; side < geometry.numHeads; side++)
            {
                int actualSectors = 0;
                for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
                {
                    Sector sector = image.get(track, side, sectorId);
                    if (sector != null)
                        actualSectors++;
                }

                Bytes trackHeader = new Bytes(0x000c + 0x0012 * actualSectors);
                ByteWriter trackHeaderWriter = new ByteWriter(trackHeader);

                trackHeaderWriter.writeLe16(0x000c); /* offset of sector headers */
                trackHeaderWriter.writeLe16(0x0012); /* length of each sector descriptor */
                trackHeaderWriter.writeLe16(actualSectors);
                trackHeaderWriter.write8(dataRate.getNumber());
                trackHeaderWriter.write8(recordingMode.getNumber());
                trackHeaderWriter.write8(0);    /* format gap length */
                trackHeaderWriter.write8(0);    /* filler byte */
                trackHeaderWriter.writeLe16(0); /* approximate track length */

                for (int sectorId = 0; sectorId < geometry.numSectors; sectorId++)
                {
                    Sector sector = image.get(track, side, sectorId);
                    if (sector != null)
                    {
                        int sectorLabel =
                                (('S') << 24) | ((track & 0xff) << 16) | (side << 8) | sectorId;
                        int sectorAddress = ldbs.put(sector.data, sectorLabel);

                        trackHeaderWriter.write8(track);
                        trackHeaderWriter.write8(side);
                        trackHeaderWriter.write8(sectorId);
                        trackHeaderWriter.write8(0); /* power-of-two size */
                        trackHeaderWriter.write8((sector.status == Sector.Status.OK) ?
                                0x00 :
                                0x20); /* 8272 status 1 */
                        trackHeaderWriter.write8(0); /* 8272 status 2 */
                        trackHeaderWriter.write8(1); /* number of copies */
                        trackHeaderWriter.write8(0); /* filler byte */
                        trackHeaderWriter.writeLe32(sectorAddress);
                        trackHeaderWriter.writeLe16(0); /* trailing bytes */
                        trackHeaderWriter.writeLe16(0); /* approximate offset */
                        trackHeaderWriter.writeLe16(sector.data.size());
                    }
                }

                int trackLabel =
                        (('T') << 24) | ((track & 0xff) << 16) | ((track >> 8) << 8) | side;
                int trackHeaderAddress = ldbs.put(trackHeader, trackLabel);
                trackDirectoryWriter.writeBe32(trackLabel);
                trackDirectoryWriter.writeLe32(trackHeaderAddress);
                trackDirectorySize++;
            }
        }

        trackDirectoryWriter.seek(0);
        trackDirectoryWriter.writeLe16(trackDirectorySize);

        int trackDirectoryAddress = ldbs.put(trackDirectory, Ldbs.TRACK_BLOCK);
        Bytes data = ldbs.write(trackDirectoryAddress);
        data.writeToFile(config.getFilename());
    }
}
