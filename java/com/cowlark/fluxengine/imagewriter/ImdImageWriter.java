package com.cowlark.fluxengine.imagewriter;

import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.ByteWriter;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.time.LocalDateTime;
import java.time.format.DateTimeFormatter;

/**
 * Writes an IMD (ImageDisk) sector image, ported from
 * lib/imagewriter/imdimagewriter.cc.
 */
public class ImdImageWriter extends ImageWriter
{
    private static final String LABEL = "IMD archive by fluxengine on";
    private static final int SEC_CYL_MAP_FLAG = 0x80;
    private static final int SEC_HEAD_MAP_FLAG = 0x40;
    private static final int END_OF_FILE = 0x1A;

    public ImdImageWriter(ImageWriterProto config)
    {
        super(config);
    }

    private static int getModulationAndSpeed(int flags, ImdOutputProto.RecordingMode mode)
    {
        if (flags == 0)
        {
            throw new FluxEngineException(
                    "Can't write IMD files with this speed " + flags + ", and modulation " + mode +
                            ". Did you read a real disk?");
        } else
        {
            flags = (int) (1000000.0 / flags);
        }

        if ((flags > 950) && (flags < 1050)) /* HD disk */
        {
            /* 500 kbps */
            if (mode == ImdOutputProto.RecordingMode.RECMODE_FM)
            {
                return 0;
            } else
            {
                return 3;
            }
        } else if ((flags > 1475) && (flags < 1575)) /* SD disk */
        {
            /* 300 kbps */
            if (mode == ImdOutputProto.RecordingMode.RECMODE_FM)
            {
                return 1;
            } else
            {
                return 4;
            }
        } else if ((flags > 1900) && (flags < 2100)) /* DD disk */
        {
            /* 250 kbps */
            if (mode == ImdOutputProto.RecordingMode.RECMODE_FM)
            {
                return 2;
            } else
            {
                return 5;
            }
        } else
        {
            throw new FluxEngineException(
                    "IMD: Can't write IMD files with this speed " + flags + ", and modulation " +
                            mode + ". Try another format.");
        }
    }

    private static int setSectorSize(int flags)
    {
        switch (flags)
        {
            case 128:
                return 0;
            case 256:
                return 1;
            case 512:
                return 2;
            case 1024:
                return 3;
            case 2048:
                return 4;
            case 4096:
                return 5;
            case 8192:
                return 6;
        }
        throw new FluxEngineException("IMD: Sector size " + flags +
                " not in standard range (128, 256, 512, 1024, 2048, 4096, 8192).");
    }

    @Override
    public void writeImage(Image image)
    {
        Geometry geometry = image.getGeometry();
        int numHeads;
        int numSectors;
        int numBytes;
        int numSectorsInTrack = 0;

        numHeads = geometry.numHeads;
        numSectors = geometry.numSectors;
        numBytes = geometry.sectorSize;

        Bytes imagenew = new Bytes();
        ByteWriter bw = imagenew.writer();

        ImdOutputProto.DataRate dataRate = config.getImd().getDataRate();
        if (dataRate == ImdOutputProto.DataRate.RATE_GUESS)
        {
            dataRate = (geometry.numSectors > 10) ?
                    ImdOutputProto.DataRate.RATE_HD :
                    ImdOutputProto.DataRate.RATE_DD;
            if (geometry.sectorSize <= 256)
                dataRate = ImdOutputProto.DataRate.RATE_SD;
            System.out.println("IMD: guessing data rate as " + dataRate);
        }

        ImdOutputProto.RecordingMode recordingMode = config.getImd().getRecordingMode();
        if (recordingMode == ImdOutputProto.RecordingMode.RECMODE_GUESS)
        {
            recordingMode = ImdOutputProto.RecordingMode.RECMODE_MFM;
            System.out.println("IMD: guessing recording mode as " + recordingMode);
        }

        String comment = config.getImd().getComment();
        if (comment.length() == 0)
        {
            comment = LABEL;
            comment = comment + " date: " + LocalDateTime
                    .now()
                    .format(DateTimeFormatter.ofPattern("E MMM d HH:mm:ss yyyy"));
        } else
        {
            comment = "IMD " + comment;
        }
        bw.seek(0);

        bw.write(comment.getBytes(java.nio.charset.StandardCharsets.US_ASCII));
        bw.write8(END_OF_FILE);
        String sectorSkew = "";
        int statusSector = 1;
        boolean blnOptionalCylinderMap = false;
        boolean blnOptionalHeadMap = false;

        /* Write the actual sector data. */
        for (int track = 0; track < geometry.numCylinders; track++)
        {
            for (int head = 0; head < numHeads; head++)
            {
                int sectorIdBase = 1; /* IMD starts sector numbering with 1 */
                int sectorId = 0;
                int modeValue = 0;
                int headerTrack = 0;
                int headerHead = 0;
                int headerNumSectors = 0;
                int headerSectorSize = 0;
                Sector sector = image.get(track, head, sectorId + 1);
                if (sector == null)
                {
                    /* sector 0 doesnt exist exit with error */
                    statusSector = 0;
                    System.out.printf(
                            "IMD: sector %d not found on track %d, head %d%n",
                            sectorId + 1,
                            track,
                            head);
                    break;
                } else
                {
                    /* Get the header information */
                    numBytes = sector.data.size();
                    headerTrack = track;
                    headerHead = head;
                    headerSectorSize = setSectorSize(numBytes);
                    sectorSkew = "";
                    numSectorsInTrack = 0;
                    double RATE = 0;
                    if (sector.clockNs > 0)
                    {
                        RATE = 1000000.0 / sector.clockNs;
                    } else
                    {
                        switch (dataRate)
                        {
                            case RATE_HD:
                                RATE = 1000;
                                break;
                            case RATE_SD:
                                RATE = 1500;
                                break;
                            case RATE_DD:
                                RATE = 2000;
                                break;
                            case RATE_GUESS:
                                break;
                        }
                    }
                    modeValue = getModulationAndSpeed((int) RATE, recordingMode);
                }
                /* determine number of sectors in track */
                for (int i = 0; i < numSectors; i++)
                {
                    Sector s = image.get(track, head, i + 1);
                    if (s == null)
                    {
                        break;
                    } else
                    {
                        numSectorsInTrack++;
                    }
                }
                /* determine sector skew and if there are optional cylinder maps
                 * or head maps */
                for (int i = 0; i < numSectorsInTrack; i++)
                {
                    Sector s = image.get(track, head, i + 1);
                    if (s == null)
                    {
                        break;
                    } else
                    {
                        sectorSkew = sectorSkew + (char) ((i + sectorIdBase) + '0');
                        if (s.physicalLocation != null &&
                                ((s.physicalLocation.cylinder() != s.logicalLocation.cylinder()) ||
                                        (s.physicalLocation.head() != s.logicalLocation.head())))
                            blnOptionalHeadMap = true;
                    }
                }
                bw.write8(modeValue); /* 1 byte ModeValue */
                bw.write8(track);     /* 1 byte Cylinder */
                /* are there optional cylinder or head maps? */
                if (blnOptionalCylinderMap)
                {
                    headerHead = headerHead ^ SEC_CYL_MAP_FLAG;
                }
                if (blnOptionalHeadMap)
                {
                    headerHead = headerHead ^ SEC_HEAD_MAP_FLAG;
                }
                bw.write8(head);        /* 1 byte Head */
                bw.write8(numSectorsInTrack); /* 1 byte number of sectors */
                bw.write8(headerSectorSize);  /* 1 byte sector size */
                for (int i = 0; i < numSectorsInTrack; i++)
                {
                    bw.write8((i + sectorIdBase)); /* sector numbering map */
                }
                /* Write optional cylinder map */
                if (blnOptionalCylinderMap)
                {
                    for (int i = 0; i < numSectorsInTrack; i++)
                    {
                        Sector s = image.get(track, head, i + 1);
                        bw.write8(s.logicalLocation.cylinder());
                    }
                }

                /* Write optional sector head map */
                if (blnOptionalHeadMap)
                {
                    for (int i = 0; i < numSectorsInTrack; i++)
                    {
                        Sector s = image.get(track, head, i + 1);
                        bw.write8(s.logicalLocation.head());
                    }
                }
                /* Now read data and write to file */
                for (int i = 0; i < numSectorsInTrack; i++)
                {
                    Sector s = image.get(track, head, i + 1);
                    boolean blnCompressable = false;
                    Bytes sectordata = new Bytes(numBytes);
                    Bytes compressed = new Bytes(1);
                    int byte0 = 0;
                    int bytePrevious = 0;
                    if (s == null)
                    {
                        statusSector = 0;
                        break;
                    } else
                    {
                        ByteReader br = s.data.iterator();
                        int j;
                        /* determine if all bytes are the same -> compress */
                        for (j = 0; j < numBytes; j++)
                        {
                            byte0 = br.read8();
                            if (j == 0)
                            {
                                bytePrevious = byte0;
                            }
                            if (bytePrevious == byte0)
                            {
                                blnCompressable = true;
                            } else
                            {
                                blnCompressable = false;
                                break;
                            }
                        }
                        switch (s.status)
                        {
                            case MISSING:
                                statusSector = 0;
                                break;

                            case OK:
                                if (blnCompressable)
                                {
                                    statusSector = 2;
                                } else
                                {
                                    statusSector = 1;
                                }
                                break;
                            case DATA_MISSING:
                                statusSector = 3;
                                break;
                            case BAD_CHECKSUM:
                                statusSector = 5;
                                break;

                            default:
                                throw new FluxEngineException(
                                        "IMD: Don't understand IMD files with sector status " +
                                                statusSector);
                        }
                        bw.write8(statusSector); /* 1 byte status sector */
                        if (blnCompressable)
                        {
                            bw.write8(byte0);
                            blnCompressable = false;
                        } else
                        {
                            bw.write(s.data);
                        }
                        numSectors = numSectorsInTrack;
                    }
                    blnOptionalCylinderMap = false;
                    blnOptionalHeadMap = false;
                }
            }
        }
        imagenew.writeToFile(config.getFilename());
        Logger.logf(
                "IMD: Written %d tracks, %d heads, %d sectors, %d bytes per " +
                        "sector, %d kB total%n",
                geometry.numCylinders,
                numHeads,
                numSectors,
                numBytes,
                imagenew.size() / 1024);
    }
}
