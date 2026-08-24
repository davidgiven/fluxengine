package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ImageReaderWriterType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import org.apache.commons.csv.CSVFormat;
import org.apache.commons.csv.CSVPrinter;
import org.apache.commons.csv.QuoteMode;
import java.io.IOException;
import java.nio.charset.StandardCharsets;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * Writes sector images to disk, ported from
 * lib/imagewriter/imagewriter.{h,cc}.
 */
public abstract class ImageWriter implements AutoCloseable
{
    /* The CSV report quotes its header fields but leaves the data fields
     * bare, so two formats are needed; both end records with a bare \n. */
    private static final CSVFormat CSV_HEADER_FORMAT = CSVFormat.DEFAULT
            .builder()
            .setQuoteMode(QuoteMode.ALL)
            .setRecordSeparator("\n")
            .build();
    private static final CSVFormat CSV_DATA_FORMAT =
            CSVFormat.DEFAULT.builder().setRecordSeparator("\n").build();

    protected final ImageWriterProto config;

    public ImageWriter(ImageWriterProto config)
    {
        this.config = config;
    }

    public static ImageWriter create(ConfigProto config)
    {
        ImageWriterProto writerConfig = config.getImageWriter();
        switch (writerConfig.getType())
        {
            case IMAGETYPE_IMG:
                return new ImgImageWriter(writerConfig, config);
            case IMAGETYPE_D64:
                return new D64ImageWriter(writerConfig);
            case IMAGETYPE_LDBS:
                return new LdbsImageWriter(writerConfig);
            case IMAGETYPE_DISKCOPY:
                return new DiskCopyImageWriter(writerConfig);
            case IMAGETYPE_NSI:
                return new NsiImageWriter(writerConfig);
            case IMAGETYPE_RAW:
                return new RawImageWriter(writerConfig);
            case IMAGETYPE_D88:
                return new D88ImageWriter(writerConfig);
            case IMAGETYPE_IMD:
                return new ImdImageWriter(writerConfig);
            default:
                throw new FluxEngineException("no image writer configured");
        }
    }

    @Override
    public void close() throws Exception
    {

    }

    protected ImageWriterProto getWriterConfig()
    {
        return config;
    }

    public void writeCsv(Image image, String filename)
    {
        try (CSVPrinter printer = new CSVPrinter(
                Files.newBufferedWriter(Path.of(filename), StandardCharsets.UTF_8),
                CSV_DATA_FORMAT);
                CSVPrinter header = new CSVPrinter(printer.getOut(), CSV_HEADER_FORMAT))
        {
            header.printRecord(
                    "Physical track",
                    "Physical side",
                    "Logical sector",
                    "Logical track",
                    "Logical side",
                    "Clock (ns)",
                    "Header start (ns)",
                    "Header end (ns)",
                    "Data start (ns)",
                    "Data end (ns)",
                    "Raw data address (bytes)",
                    "User payload length (bytes)",
                    "Status");

            for (Sector sector : image)
            {
                printer.printRecord(
                        sector.physicalLocation != null ? sector.physicalLocation.cylinder() : -1,
                        sector.physicalLocation != null ? sector.physicalLocation.head() : -1,
                        sector.location.logicalSector(),
                        sector.location.logicalCylinder(),
                        sector.location.logicalHead(),
                        sector.clockNs,
                        sector.headerStartTimeNs,
                        sector.headerEndTimeNs,
                        sector.dataStartTimeNs,
                        sector.dataEndTimeNs,
                        sector.position,
                        sector.data.size(),
                        Sector.statusToString(sector.status));
            }
        } catch (IOException e)
        {
            throw new FluxEngineException("cannot open CSV report file");
        }
    }

    public void printMap(Image image)
    {
        Geometry geometry = image.getGeometry();

        int badSectors = 0;
        int missingSectors = 0;
        int totalSectors = 0;

        System.out.print("     Tracks -> ");
        for (int i = 10; i < geometry.numCylinders; i += 10)
            System.out.printf("%-10d", i / 10);
        System.out.println();
        System.out.print("H.SS ");
        for (int i = 0; i < geometry.numCylinders; i++)
            System.out.print(i % 10);
        System.out.println();

        for (int side = 0; side < geometry.numHeads; side++)
        {
            int maxSector = geometry.firstSector + geometry.numSectors - 1;
            for (int sectorId = 0; sectorId <= maxSector; sectorId++)
            {
                if (sectorId < geometry.firstSector)
                    continue;

                System.out.printf("%d.%2d ", side, sectorId);
                for (int track = 0; track < geometry.numCylinders; track++)
                {
                    Sector sector = image.get(track, side, sectorId);
                    if (sector == null)
                    {
                        System.out.print('X');
                        missingSectors++;
                    } else
                    {
                        switch (sector.status)
                        {
                            case OK:
                                System.out.print('.');
                                break;

                            case BAD_CHECKSUM:
                                System.out.print('B');
                                badSectors++;
                                break;

                            case CONFLICT:
                                System.out.print('C');
                                badSectors++;
                                break;

                            default:
                                System.out.print(sector.status.ordinal());
                                break;
                        }
                    }
                    totalSectors++;
                }
                System.out.println();
            }
        }
        int goodSectors = totalSectors - missingSectors - badSectors;
        if (totalSectors == 0)
            System.out.println("No sectors in output; skipping analysis");
        else
        {
            System.out.printf(
                    "Good sectors: %d/%d (%d%%)%n",
                    goodSectors,
                    totalSectors,
                    100 * goodSectors / totalSectors);
            System.out.printf(
                    "Missing sectors: %d/%d (%d%%)%n",
                    missingSectors,
                    totalSectors,
                    100 * missingSectors / totalSectors);
            System.out.printf(
                    "Bad sectors: %d/%d (%d%%)%n",
                    badSectors,
                    totalSectors,
                    100 * badSectors / totalSectors);
        }
    }

    /* Writes a raw image. */

    public abstract void writeImage(Image image);
}
