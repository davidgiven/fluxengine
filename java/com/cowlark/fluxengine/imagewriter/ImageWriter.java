package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ImageReaderWriterType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
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
    protected final ImageWriterProto config;

    public ImageWriter(ImageWriterProto config)
    {
        this.config = config;
    }

    public static ImageWriter create(ConfigProto config)
    {
        if (!config.hasImageWriter())
            throw new FluxEngineException("no image writer configured");
        if (config.getImageWriter().getType() == ImageReaderWriterType.IMAGETYPE_IMG)
            return new ImgImageWriter(config.getImageWriter(), config);
        return create(config.getImageWriter());
    }

    public static ImageWriter create(ImageWriterProto config)
    {
        switch (config.getType())
        {
            case IMAGETYPE_IMG:
                return notImplemented("img");
            case IMAGETYPE_D64:
                return new D64ImageWriter(config);
            case IMAGETYPE_LDBS:
                return notImplemented("ldbs");
            case IMAGETYPE_DISKCOPY:
                return new DiskCopyImageWriter(config);
            case IMAGETYPE_NSI:
                return new NsiImageWriter(config);
            case IMAGETYPE_RAW:
                return new RawImageWriter(config);
            case IMAGETYPE_D88:
                return new D88ImageWriter(config);
            case IMAGETYPE_IMD:
                return new ImdImageWriter(config);
            default:
                throw new FluxEngineException("bad output image config");
        }
    }

    private static ImageWriter notImplemented(String name)
    {
        throw new FluxEngineException(name + " image writer is not implemented yet");
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
        StringBuilder f = new StringBuilder();
        f.append("\"Physical track\",")
                .append("\"Physical side\",")
                .append("\"Logical sector\",")
                .append("\"Logical track\",")
                .append("\"Logical side\",")
                .append("\"Clock (ns)\",")
                .append("\"Header start (ns)\",")
                .append("\"Header end (ns)\",")
                .append("\"Data start (ns)\",")
                .append("\"Data end (ns)\",")
                .append("\"Raw data address (bytes)\",")
                .append("\"User payload length (bytes)\",")
                .append("\"Status\"")
                .append("\n");

        for (Sector sector : image)
        {
            f.append(sector.physicalLocation != null ? sector.physicalLocation.cylinder() : -1)
                    .append(',');
            f.append(sector.physicalLocation != null ? sector.physicalLocation.head() : -1)
                    .append(',');
            f.append(sector.location.logicalSector()).append(',');
            f.append(sector.location.logicalCylinder()).append(',');
            f.append(sector.location.logicalHead()).append(',');
            f.append(sector.clockNs).append(',');
            f.append(sector.headerStartTimeNs).append(',');
            f.append(sector.headerEndTimeNs).append(',');
            f.append(sector.dataStartTimeNs).append(',');
            f.append(sector.dataEndTimeNs).append(',');
            f.append(sector.position).append(',');
            f.append(sector.data.size()).append(',');
            f.append(Sector.statusToString(sector.status));
            f.append("\n");
        }

        try
        {
            Files.writeString(Path.of(filename), f.toString(), StandardCharsets.UTF_8);
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
            System.out.printf("Good sectors: %d/%d (%d%%)%n",
                    goodSectors,
                    totalSectors,
                    100 * goodSectors / totalSectors);
            System.out.printf("Missing sectors: %d/%d (%d%%)%n",
                    missingSectors,
                    totalSectors,
                    100 * missingSectors / totalSectors);
            System.out.printf("Bad sectors: %d/%d (%d%%)%n",
                    badSectors,
                    totalSectors,
                    100 * badSectors / totalSectors);
        }
    }

    /* Writes a raw image. */

    public abstract void writeImage(Image image);
}
