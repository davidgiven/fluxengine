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
public class D88ImageReader extends ImageReader
{
    public D88ImageReader(ImageReaderProto config)
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

        /* The DIM header technically has a bit field for sectors present,
         * however it is currently ignored by this reader */
        Bytes header = data.slice(0, 0x24); /* read first entry of track table as well */

        String diskName = header.slice(0, 0x16).toString();
        if (diskName.length() > 0 && diskName.charAt(0) != 0)
            Logger.logf("D88: disk name: " + diskName);

        ByteReader headerReader = new ByteReader(header);

        int mediaFlag = headerReader.seek(0x1b).read8();
        int fileSize = data.size();

        int diskSize = headerReader.seek(0x1c).readLe32();

        if (diskSize > fileSize)
            Logger.logf("D88: found multiple disk images. Only using first");

        int trackTableEnd = headerReader.seek(0x20).readLe32();
        int trackTableSize = trackTableEnd - 0x20;

        ByteReader trackTableReader = new ByteReader(data.slice(0x20, trackTableSize));

        ConfigProto.Builder extra = ConfigProto.newBuilder();
        IbmEncoderProto.Builder ibm = extra.getEncoderBuilder().getIbmBuilder();
        int clockRate = 500;
        if (mediaFlag == 0x20)
        {
            extra.getDriveBuilder().setHighDensity(true);
            extra.getLayoutBuilder().setFormatType(FormatType.FORMATTYPE_80TRACK);
        } else
        {
            clockRate = 300;
            extra.getDriveBuilder().setHighDensity(false);
            extra.getLayoutBuilder().setFormatType(FormatType.FORMATTYPE_40TRACK);
        }

        com.cowlark.fluxengine.config.LayoutProto.Builder layout = extra.getLayoutBuilder();
        Image image = new Image();
        ByteReader br = new ByteReader(data);
        br.seek(0x20 + trackTableSize);
        for (int track = 0; track < trackTableSize / 4; track++)
        {
            int trackOffset = trackTableReader.seek(track * 4).readLe32();
            if (trackOffset == 0)
                continue;

            int currentTrackTrack = -1;
            int currentSectorsInTrack =
                    0xffff; /* don't know # of sectors until we read the first one */
            int trackSectorSize = -1;
            int trackMfm = -1;

            IbmEncoderProto.TrackdataProto.Builder trackdata = ibm.addTrackdataBuilder();
            trackdata.setTargetClockPeriodUs(1e3 / clockRate);
            trackdata.setTargetRotationalPeriodMs(167);

            com.cowlark.fluxengine.config.LayoutProto.LayoutdataProto.Builder layoutdata =
                    layout.addLayoutdataBuilder();
            com.cowlark.fluxengine.config.SectorListProto.Builder physical =
                    layoutdata.getPhysicalBuilder();

            for (int sectorInTrack = 0; sectorInTrack < currentSectorsInTrack; sectorInTrack++)
            {
                ByteReader sectorHeaderReader = new ByteReader(br.read(0x10));
                int cyl = sectorHeaderReader.seek(0).read8();
                int head = sectorHeaderReader.seek(1).read8();
                int sectorId = sectorHeaderReader.seek(2).read8();
                int sectorSize = 128 << sectorHeaderReader.seek(3).read8();
                int sectorsInTrack = sectorHeaderReader.seek(4).readLe16();
                int fm = sectorHeaderReader.seek(6).read8();
                int ddam = sectorHeaderReader.seek(7).read8();
                int fddStatusCode = sectorHeaderReader.seek(8).read8();
                int rpm = sectorHeaderReader.seek(13).read8();
                int dataLength = sectorHeaderReader.seek(14).readLe16();
                if (dataLength < sectorSize)
                {
                    dataLength = sectorSize;
                }
                /* D88 provides much more sector information that is currently
                 * ignored */
                if (ddam != 0)
                    throw new FluxEngineException("D88: nonzero ddam currently unsupported");
                if (rpm != 0)
                    throw new FluxEngineException("D88: 1.44MB 300rpm formats currently " +
                            "unsupported");
                if (fddStatusCode != 0)
                    throw new FluxEngineException(
                            "D88: nonzero fdd status codes are currently unsupported");
                if (currentSectorsInTrack == 0xffff)
                {
                    currentSectorsInTrack = sectorsInTrack;
                } else if (currentSectorsInTrack != sectorsInTrack)
                {
                    throw new FluxEngineException("D88: mismatched number of sectors in track");
                }
                if (currentTrackTrack < 0)
                {
                    currentTrackTrack = cyl;
                } else if (currentTrackTrack != cyl)
                {
                    throw new FluxEngineException(
                            "D88: all sectors in a track must belong to the same track");
                }
                if (trackSectorSize < 0)
                {
                    trackSectorSize = sectorSize;
                    /* this is the first sector we've read, use its settings for
                     * per-track data */

                    layoutdata.setTrack(cyl);
                    layoutdata.setSide(head);
                    layoutdata.setSectorSize(sectorSize);

                    trackdata.setTrack(cyl);
                    trackdata.setHead(head);
                    trackdata.setUseFm(fm != 0);
                    if (fm != 0)
                    {
                        trackdata.setGapFillByte(0xffff);
                        trackdata.setIdamByte(0xf57e);
                        trackdata.setDamByte(0xf56f);
                    }
                    /* create timings to approximately match N88-BASIC */
                    if (clockRate == 300)
                    {
                        if (sectorSize <= 256)
                        {
                            trackdata.setGap0(0x1b);
                            trackdata.setGap2(0x14);
                            trackdata.setGap3(0x1b);
                        }
                    } else
                    {
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
                    }
                } else if (trackSectorSize != sectorSize)
                {
                    throw new FluxEngineException(
                            "D88: multiple sector sizes per track are currently unsupported");
                }

                Bytes sectorData = br.read(sectorSize);
                br.skip(dataLength - sectorSize);
                physical.addSector(sectorId);
                Sector sector = image.put(cyl, head, sectorId);
                sector.status = Sector.Status.OK;
                sector.data = sectorData;
            }

            if (mediaFlag != 0x20)
            {
                IbmEncoderProto.TrackdataProto.Builder trackdata2 = ibm.addTrackdataBuilder();
                trackdata2.setTargetClockPeriodUs(1e3 / clockRate);
                trackdata2.setTargetRotationalPeriodMs(167);
            }
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf(
                "D88: read " + geometry.numCylinders + " tracks, " + geometry.numHeads + " sides");

        layout.setTracks(geometry.numCylinders);
        layout.setSides(geometry.numHeads);

        extraConfig = extra.build();
        return image;
    }
}
