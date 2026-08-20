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

/* Reader based on this partial documentation of the FDI format:
 * https://www.pc98.org/project/doc/hdi.html
 */
public class FdiImageReader extends ImageReader
{
    public FdiImageReader(ImageReaderProto config, ConfigProto fullConfig)
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

        ByteReader headerReader = new ByteReader(data.slice(0, 32));
        if (headerReader.seek(0).readLe32() != 0)
            throw new FluxEngineException("FDI: could not find FDI header, is this a FDI file?");

        /* we currently don't use fddType but it could be used to automatically
         * select profile parameters in the future */
        int fddType = headerReader.seek(4).readLe32();
        int headerSize = headerReader.seek(0x08).readLe32();
        int sectorSize = headerReader.seek(0x10).readLe32();
        int sectorsPerTrack = headerReader.seek(0x14).readLe32();
        int sides = headerReader.seek(0x18).readLe32();
        int tracks = headerReader.seek(0x1c).readLe32();

        ByteReader br = new ByteReader(data.slice(headerSize));

        Image image = new Image();
        int trackCount = 0;
        for (int track = 0; track < tracks; track++)
        {
            if (br.eof())
                break;

            for (int side = 0; side < sides; side++)
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
            switch (fddType)
            {
                case 0x90:
                    Logger.logf("FDI: automatically setting format to 1.2MB (1024 byte sectors)");
                    trackdata.setTargetRotationalPeriodMs(167);
                    layoutdata.setSectorSize(1024);
                    for (int i = 0; i < 9; i++)
                        physical.addSector(i);
                    break;

                case 0x30:
                    Logger.logf("FDI: automatically setting format to 1.44MB");
                    trackdata.setTargetRotationalPeriodMs(200);
                    layoutdata.setSectorSize(512);
                    for (int i = 0; i < 18; i++)
                        physical.addSector(i);
                    break;

                default:
                    throw new FluxEngineException(String.format(
                            "FDI: unknown fdd type 0x%02x, could not determine write " +
                                    "profile automatically", fddType));
            }
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf("FDI: read " + geometry.numCylinders + " tracks, " + geometry.numHeads +
                " sides, " + (data.size() - headerSize) / 1024 + " kB total");

        layout.setTracks(geometry.numCylinders);
        layout.setSides(geometry.numHeads);

        extraConfig = extra.build();
        return image;
    }
}
