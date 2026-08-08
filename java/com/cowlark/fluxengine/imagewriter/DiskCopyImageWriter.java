package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.nio.charset.StandardCharsets;

/**
 * Writes a DiskCopy 4.2 sector image, ported from
 * lib/imagewriter/diskcopyimagewriter.cc.
 */
public class DiskCopyImageWriter extends ImageWriter
{
    private static final String LABEL = "FluxEngine image";

    public DiskCopyImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    private static void writeAndUpdateChecksum(
            ByteWriter bw, int[] checksum, Bytes data)
    {
        ByteReader br = data.iterator();
        while (!br.eof())
        {
            int i = br.readBe16();
            checksum[0] += i;
            checksum[0] = (checksum[0] >>> 1) | (checksum[0] << 31);
            bw.writeBe16(i);
        }
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();

        boolean mfm = false;

        switch (geometry.sectorSize)
        {
            case 524:
                /* GCR disk */
                break;

            case 512:
                /* MFM disk */
                mfm = true;
                break;

            default:
                throw new FluxEngineException(
                        "this image is not compatible with the DiskCopy 4.2 format");
        }
        final boolean isMfm = mfm;

        System.out.println("DC42: writing DiskCopy 4.2 image");
        System.out.printf(
                "DC42: %d tracks, %d sides, %d sectors, %d bytes per sector; %s%n",
                geometry.numCylinders,
                geometry.numHeads,
                geometry.numSectors,
                geometry.sectorSize,
                isMfm ? "MFM" : "GCR");

        java.util.function.IntUnaryOperator sectorsPerTrack = track ->
        {
            if (isMfm)
                return geometry.numSectors;

            if (track < 16)
                return 12;
            if (track < 32)
                return 11;
            if (track < 48)
                return 10;
            if (track < 64)
                return 9;
            return 8;
        };

        Bytes data = new Bytes();
        ByteWriter bw = data.writer();

        /* Write the actual sector data. */

        int[] dataChecksum = {0};
        int[] tagChecksum = {0};
        int offset = 0x54;
        int sectorDataStart = offset;
        for (int track = 0; track < geometry.numCylinders; track++)
        {
            for (int side = 0; side < geometry.numHeads; side++)
            {
                int sectorCount = sectorsPerTrack.applyAsInt(track);
                for (int sectorId = 0; sectorId < sectorCount; sectorId++)
                {
                    Sector sector = image.get(track, side, sectorId);
                    if (sector != null)
                    {
                        bw.seek(offset);
                        writeAndUpdateChecksum(bw, dataChecksum, sector.data.slice(0, 512));
                    }
                    offset += 512;
                }
            }
        }
        int sectorDataEnd = offset;
        if (!mfm)
        {
            for (int track = 0; track < geometry.numCylinders; track++)
            {
                for (int side = 0; side < geometry.numHeads; side++)
                {
                    int sectorCount = sectorsPerTrack.applyAsInt(track);
                    for (int sectorId = 0; sectorId < sectorCount; sectorId++)
                    {
                        Sector sector = image.get(track, side, sectorId);
                        if (sector != null)
                        {
                            bw.seek(offset);
                            writeAndUpdateChecksum(
                                    bw, tagChecksum, sector.data.slice(512, 12));
                        }
                        offset += 12;
                    }
                }
            }
        }
        int tagDataEnd = offset;

        /* Write the header. */

        int encoding;
        int format;
        if (isMfm)
        {
            format = 0x22;
            if (geometry.numSectors == 18)
                encoding = 3;
            else
                encoding = 2;
        } else
        {
            if (geometry.numHeads == 2)
            {
                encoding = 1;
                format = 0x22;
            } else
            {
                encoding = 0;
                format = 0x02;
            }
        }

        bw.seek(0);
        bw.write8(LABEL.getBytes(StandardCharsets.US_ASCII).length);
        bw.write(LABEL.getBytes(StandardCharsets.US_ASCII));
        bw.seek(0x40);
        bw.writeBe32(sectorDataEnd - sectorDataStart); /* data size */
        bw.writeBe32(tagDataEnd - sectorDataEnd);      /* tag size */
        bw.writeBe32(dataChecksum[0]);                 /* data checksum */
        bw.writeBe32(tagChecksum[0]);                  /* tag checksum */
        bw.write8(encoding);                           /* encoding */
        bw.write8(format);                             /* format byte */
        bw.writeBe16(0x0100);                          /* magic number */

        data.writeToFile(config.getFilename());
    }
}
