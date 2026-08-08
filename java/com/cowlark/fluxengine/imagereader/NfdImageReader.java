package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.ibm.IbmEncoderProto;
import com.cowlark.fluxengine.external.FormatType;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/* Reader based on this partial documentation of the D88 format:
 * https://www.pc98.org/project/doc/d88.html
 */
public class NfdImageReader extends ImageReader
{
    public NfdImageReader(ImageReaderProto config)
    {
        super(config);
    }

    @Override
    public Image readImage()
    {
        Bytes data;
        try
        {
            data = new Bytes(Files.readAllBytes(Path.of(config.getFilename())));
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open input file");
        }

        Bytes fileId = data.slice(0, 14);
        if (fileId.equals(new Bytes("T98FDDIMAGE.R1")))
        {
            throw new FluxEngineException("NFD: r1 images are not currently supported");
        }
        if (!fileId.equals(new Bytes("T98FDDIMAGE.R0")))
        {
            throw new FluxEngineException("NFD: could not find NFD header");
        }

        ByteReader headerReader = new ByteReader(data);

        int heads = headerReader.seek(0x115).read8();
        if (heads != 2)
        {
            throw new FluxEngineException("NFD: unsupported number of heads");
        }

        ConfigProto.Builder extra = ConfigProto.newBuilder();
        IbmEncoderProto.Builder ibm = extra.getEncoderBuilder().getIbmBuilder();
        com.cowlark.fluxengine.config.LayoutProto.Builder layout = extra.getLayoutBuilder();
        Logger.logf("NFD: HD 1.2MB mode");
        Logger.logf("NFD: forcing high density mode");
        extra.getDriveBuilder().setHighDensity(true);
        extra.getLayoutBuilder().setFormatType(FormatType.FORMATTYPE_80TRACK);

        Image image = new Image();
        ByteReader br = new ByteReader(data);
        br.seek(0x10a10);
        for (int track = 0; track < 163; track++)
        {
            IbmEncoderProto.TrackdataProto.Builder trackdata = ibm.addTrackdataBuilder();
            trackdata.setTargetClockPeriodUs(2);
            trackdata.setTargetRotationalPeriodMs(167);

            com.cowlark.fluxengine.config.LayoutProto.LayoutdataProto.Builder layoutdata =
                    layout.addLayoutdataBuilder();
            com.cowlark.fluxengine.config.SectorListProto.Builder physical =
                    layoutdata.getPhysicalBuilder();
            int currentTrackTrack = -1;
            int currentTrackHead = -1;
            int trackSectorSize = -1;

            for (int sectorInTrack = 0; sectorInTrack < 26; sectorInTrack++)
            {
                ByteReader sectorHeaderReader = new ByteReader(data.slice(
                        0x120 + track * 26 * 16 + sectorInTrack * 16,
                        16));
                int cyl = sectorHeaderReader.seek(0).read8();
                int head = sectorHeaderReader.seek(1).read8();
                int sectorId = sectorHeaderReader.seek(2).read8();
                int sectorSize = 128 << sectorHeaderReader.seek(3).read8();
                int mfm = sectorHeaderReader.seek(4).read8();
                int ddam = sectorHeaderReader.seek(5).read8();
                int status = sectorHeaderReader.seek(6).read8();
                sectorHeaderReader.skip(9); /* skip ST0, ST1, ST2, PDA, reserved(5) */
                if (cyl == 0xFF)
                    continue;
                if (ddam != 0)
                    throw new FluxEngineException("NFD: nonzero ddam currently unsupported");
                if (status != 0)
                    throw new FluxEngineException(
                            "NFD: nonzero fdd status codes are currently unsupported");
                if (currentTrackTrack < 0)
                {
                    currentTrackTrack = cyl;
                    currentTrackHead = head;
                } else if (currentTrackTrack != cyl)
                {
                    throw new FluxEngineException(
                            "NFD: all sectors in a track must belong to the same track");
                } else if (currentTrackHead != head)
                {
                    throw new FluxEngineException(
                            "NFD: all sectors in a track must belong to the same head");
                }
                if (trackSectorSize < 0)
                {
                    trackSectorSize = sectorSize;
                    /* this is the first sector we've read, use its settings for
                     * per-track data */
                    trackdata.setTrack(cyl);
                    trackdata.setHead(head);
                    layoutdata.setTrack(cyl);
                    layoutdata.setSide(head);
                    layoutdata.setSectorSize(sectorSize);
                    trackdata.setUseFm(mfm == 0);
                    if (mfm == 0)
                    {
                        trackdata.setGapFillByte(0xffff);
                        trackdata.setIdamByte(0xf57e);
                        trackdata.setDamByte(0xf56f);
                    }
                    /* create timings to approximately match N88-BASIC */
                    if (sectorSize <= 128)
                    {
                        trackdata.setGap0(0x1b);
                        trackdata.setGap2(0x09);
                        trackdata.setGap3(0x1b);
                    } else if (sectorSize <= 256)
                    {
                        trackdata.setGap0(0x36);
                        trackdata.setGap3(0x36);
                    }
                } else if (trackSectorSize != sectorSize)
                {
                    throw new FluxEngineException(
                            "NFD: multiple sector sizes per track are currently unsupported");
                }
                Bytes sectorData = br.read(sectorSize);
                physical.addSector(sectorId);
                Sector sector = image.put(cyl, head, sectorId);
                sector.status = Sector.Status.OK;
                sector.data = sectorData;
            }
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf(
                "NFD: read " + geometry.numCylinders + " tracks, " + geometry.numHeads + " sides");

        extraConfig = extra.build();
        return image;
    }
}
