package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.external.Crc;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/* The best description of the Teledisk format I've found is available here:
 *
 * https://web.archive.org/web/20210420230238/http://dunfield.classiccmp.org/img47321/td0notes.txt
 */
public class Td0ImageReader extends ImageReader
{
    private static final int TD0_ENCODING_RAW = 0;
    private static final int TD0_ENCODING_REPEATED = 1;
    private static final int TD0_ENCODING_RLE = 2;

    private static final int TD0_FLAG_DUPLICATE = 0x01;
    private static final int TD0_FLAG_CRC_ERROR = 0x02;
    private static final int TD0_FLAG_DELETED = 0x04;
    private static final int TD0_FLAG_SKIPPED = 0x10;
    private static final int TD0_FLAG_IDNODATA = 0x20;
    private static final int TD0_FLAG_DATANOID = 0x40;

    public Td0ImageReader(ImageReaderProto config)
    {
        super(config);
    }

    @Override
    public Image readImage()
    {
        Bytes input;
        try
        {
            input = new Bytes(Files.readAllBytes(Path.of(config.getFilename())));
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open input file");
        }
        ByteReader br = new ByteReader(input);

        int signature = br.readBe16();
        br.skip(2); /* sequence and checksequence */
        int version = br.read8();
        br.skip(2); /* data rate, drive type */
        int stepping = br.read8();
        br.skip(1); /* sparse flag */
        int sides = (br.read8() == 1) ? 1 : 2;
        int headerCrc = br.readLe16();

        int gotCrc = Crc.crc16(0xa097, 0, input.slice(0, 10));
        if (gotCrc != headerCrc)
            throw new FluxEngineException("TD0: header checksum mismatch");
        if (signature != 0x5444)
            throw new FluxEngineException(
                    "TD0: unsupported file type (only uncompressed files are supported for now)");

        String comment = "(no comment)";
        if ((stepping & 0x80) != 0)
        {
            /* Comment block */

            br.skip(2); /* comment CRC */
            int length = br.readLe16();
            br.skip(6); /* timestamp */
            comment = br.read(length).toString();
            comment = comment.replace('\0', '\n');

            /* Strip trailing whitespace */

            int end = comment.length();
            while (end > 0 && Character.isWhitespace(comment.charAt(end - 1)))
                end--;
            comment = comment.substring(0, end);
        }

        Logger.logf("TD0: TeleDisk " + version / 10 + "." + version % 10 + ": " + comment);

        int totalSize = 0;
        Image image = new Image();
        for (; ; )
        {
            /* Read track header */

            int sectorCount = br.read8();
            if (sectorCount == 0xff)
                break;

            int physicalCylinder = br.read8();
            int physicalHead = br.read8() & 1;
            br.skip(1); /* crc */

            for (int i = 0; i < sectorCount; i++)
            {
                /* Read sector */

                int logicalCylinder = br.read8();
                int logicalHead = br.read8();
                int sectorId = br.read8();
                int sectorSizeEncoded = br.read8();
                int sectorSize = 128 << sectorSizeEncoded;
                int flags = br.read8();
                br.skip(1); /* CRC */

                int dataSize = br.readLe16();
                Bytes encodedData = br.read(dataSize);
                ByteReader bre = new ByteReader(encodedData);
                int encoding = bre.read8();

                Bytes data;
                if ((flags & (TD0_FLAG_SKIPPED | TD0_FLAG_IDNODATA)) == 0)
                {
                    switch (encoding)
                    {
                        case TD0_ENCODING_RAW:
                            data = encodedData.slice(1);
                            break;

                        case TD0_ENCODING_REPEATED:
                        {
                            data = new Bytes(0);
                            ByteWriter bw = data.writer();
                            while (!bre.eof())
                            {
                                int pattern = bre.readLe16();
                                int count = bre.readLe16();
                                while (count-- != 0)
                                    bw.writeLe16(pattern);
                            }
                            break;
                        }

                        case TD0_ENCODING_RLE:
                        {
                            data = new Bytes(0);
                            ByteWriter bw = data.writer();
                            while (!bre.eof())
                            {
                                int length = bre.read8() * 2;
                                if (length == 0)
                                {
                                    /* Literal block */

                                    length = bre.read8();
                                    bw.write(bre.read(length));
                                } else
                                {
                                    /* Repeated block */

                                    int count = bre.read8();
                                    Bytes b = bre.read(length);
                                    while (count-- != 0)
                                        bw.write(b);
                                }
                            }
                            break;
                        }

                        default:
                            data = new Bytes(0);
                            break;
                    }
                } else
                    data = new Bytes(0);

                Sector sector = image.put(logicalCylinder, logicalHead, sectorId);
                sector.status = Sector.Status.OK;
                sector.data = data.slice(0, sectorSize);
                totalSize += sectorSize;
            }
        }

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        Logger.logf("TD0: found " + geometry.numCylinders + " tracks, " + geometry.numHeads +
                " sides, " + geometry.numSectors + " sectors, " + geometry.sectorSize +
                " bytes per sector, " + totalSize / 1024 + " kB total");
        return image;
    }
}
