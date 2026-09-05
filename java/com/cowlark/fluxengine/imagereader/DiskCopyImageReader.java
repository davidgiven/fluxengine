package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Reads a DiskCopy (Mac) sector image, ported from
 * lib/imagereader/diskcopyimagereader.cc.
 */
public class DiskCopyImageReader extends ImageReader
{
    public DiskCopyImageReader(ImageReaderProto config)
    {
        super(config);
    }

    private static int sectorsPerTrack(int track, int numSectors, boolean mfm)
    {
        if (mfm)
            return numSectors;

        if (track < 16)
            return 12;
        if (track < 32)
            return 11;
        if (track < 48)
            return 10;
        if (track < 64)
            return 9;
        return 8;
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
        ByteReader br = new ByteReader(data);

        br.seek(1);
        String label = br.read(data.getByte(0) & 0xff).toString();

        br.seek(0x40);
        int dataSize = br.readBe32();

        br.seek(0x50);
        int encoding = br.read8();
        int formatByte = br.read8();

        int numCylinders = 80;
        int numHeads = 2;
        int numSectors = 0;
        boolean mfm = false;

        switch (encoding)
        {
            case 0: /* GCR CLV 400kB */
                numHeads = 1;
                break;

            case 1: /* GCR CLV 800kB */
                break;

            case 2: /* MFM CAV 720kB */
                numSectors = 9;
                mfm = true;
                break;

            case 3: /* MFM CAV 1440kB */
                numSectors = 18;
                mfm = true;
                break;

            default:
                throw new FluxEngineException(
                        "don't understand DiskCopy disks of type " + encoding);
        }

        Logger.logf(
                "DC42: reading image with " + numCylinders + " tracks, " + numHeads + " heads; " +
                        (mfm ? "MFM" : "GCR") + "; " + label);

        int dataPtr = 0x54;
        int tagPtr = dataPtr + dataSize;

        Image image = new Image();
        for (int track = 0; track < numCylinders; track++)
        {
            int sectorCount = sectorsPerTrack(track, numSectors, mfm);
            for (int head = 0; head < numHeads; head++)
            {
                for (int sectorId = 0; sectorId < sectorCount; sectorId++)
                {
                    br.seek(dataPtr);
                    Bytes payload = br.read(512);
                    dataPtr += 512;

                    br.seek(tagPtr);
                    Bytes tag = br.read(12);
                    tagPtr += 12;

                    Sector sector = image.put(track, head, sectorId);
                    sector.status = Sector.Status.OK;
                    sector.data = payload.concat(tag);
                }
            }
        }

        ConfigProto.Builder extra = ConfigProto.newBuilder();
        extra.getLayoutBuilder().addLayoutdataBuilder().setSectorSize(524);
        extraConfig = extra.build();

        Geometry geometry = new Geometry();
        geometry.numCylinders = numCylinders;
        geometry.numHeads = numHeads;
        geometry.numSectors = 12;
        geometry.sectorSize = 512 + 12;
        geometry.irregular = true;
        image.setGeometry(geometry);
        return image;
    }
}
