package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.EncoderProto;
import com.cowlark.fluxengine.ibm.IbmEncoderProto;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/* Reader based on this partial documentation of the DIM format:
 * https://www.pc98.org/project/doc/dim.html
 */
public class DimImageReader extends ImageReader
{
    public DimImageReader(ImageReaderProto config, ConfigProto fullConfig)
    {
        super(config, fullConfig);
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

        Bytes header = data.slice(0, 256);
        if (!header.slice(0xAB, 13).equals(new Bytes("DIFC HEADER  ")))
            throw new FluxEngineException("DIM: could not find DIM header, is this a DIM file?");

        /* the DIM header technically has a bit field for sectors present,
         * however it is currently ignored by this reader */

        int mediaByte = header.getByte(0) & 0xff;
        int tracks;
        int sectorsPerTrack;
        int sectorSize;
        switch (mediaByte)
        {
            case 0:
                tracks = 77;
                sectorsPerTrack = 8;
                sectorSize = 1024;
                break;
            case 1:
                tracks = 80;
                sectorsPerTrack = 9;
                sectorSize = 1024;
                break;
            case 2:
                tracks = 80;
                sectorsPerTrack = 15;
                sectorSize = 512;
                break;
            case 3:
                tracks = 80;
                sectorsPerTrack = 18;
                sectorSize = 512;
                break;
            default:
                throw new FluxEngineException("DIM: unsupported media byte");
        }

        Image image = new Image();
        int trackCount = 0;
        ByteReader br = new ByteReader(data.slice(256));
        for (int track = 0; track < tracks; track++)
        {
            if (br.eof())
                break;

            for (int side = 0; side < 2; side++)
            {
                for (int sectorId = 1; sectorId <= sectorsPerTrack; sectorId++)
                {
                    Bytes sectorData = br.read(sectorSize);

                    Sector sector = image.put(track, side, sectorId);
                    sector.status = Sector.Status.OK;
                    sector.data = sectorData;
                }
            }

            trackCount++;
        }

        ConfigProto.Builder extra = ConfigProto.newBuilder();
        com.cowlark.fluxengine.config.LayoutProto.Builder layout = extra.getLayoutBuilder();
        if (fullConfig.getEncoder().getFormatCase() == EncoderProto.FormatCase.FORMAT_NOT_SET)
        {
            IbmEncoderProto.Builder ibm = extra.getEncoderBuilder().getIbmBuilder();
            IbmEncoderProto.TrackdataProto.Builder trackdata = ibm.addTrackdataBuilder();
            trackdata.setTargetClockPeriodUs(2);

            com.cowlark.fluxengine.config.LayoutProto.LayoutdataProto.Builder layoutdata =
                    layout.addLayoutdataBuilder();
            com.cowlark.fluxengine.config.SectorListProto.Builder physical =
                    layoutdata.getPhysicalBuilder();
            switch (mediaByte)
            {
                case 0x00:
                    Logger.logf("DIM: automatically setting format to 1.2MB (1024 byte sectors)");
                    trackdata.setTargetRotationalPeriodMs(167);
                    layoutdata.setSectorSize(1024);
                    for (int i = 0; i < 9; i++)
                        physical.addSector(i);
                    break;
                case 0x02:
                    Logger.logf("DIM: automatically setting format to 1.2MB (512 byte sectors)");
                    trackdata.setTargetRotationalPeriodMs(167);
                    layoutdata.setSectorSize(512);
                    for (int i = 0; i < 15; i++)
                        physical.addSector(i);
                    break;
                case 0x03:
                    Logger.logf("DIM: automatically setting format to 1.44MB");
                    trackdata.setTargetRotationalPeriodMs(200);
                    layoutdata.setSectorSize(512);
                    for (int i = 0; i < 18; i++)
                        physical.addSector(i);
                    break;
                default:
                    throw new FluxEngineException(String.format(
                            "DIM: unknown media byte 0x%02x, could not determine write " +
                                    "profile automatically",
                            mediaByte));
            }

            extra.getDecoderBuilder().getIbmBuilder();
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf("DIM: read " + geometry.numCylinders + " tracks, " + geometry.numHeads +
                " sides, " + (data.size() - 256) / 1024 + " kB total");

        layout.setTracks(geometry.numCylinders);
        layout.setSides(geometry.numHeads);

        extraConfig = extra.build();
        return image;
    }
}
