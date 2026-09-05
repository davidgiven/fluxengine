package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.ByteReader;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.Geometry;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.encoders.EncoderProto;
import com.cowlark.fluxengine.ibm.IbmEncoderProto;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.ArrayList;
import java.util.List;

public class ImdImageReader extends ImageReader
{
    private static final int SEC_CYL_MAP_FLAG = 0x80;
    private static final int SEC_HEAD_MAP_FLAG = 0x40;
    private static final int HEAD_MASK = 0x3F;
    private static final int END_OF_FILE = 0x1A;

    public ImdImageReader(ImageReaderProto config)
    {
        super(config);
    }

    private static int getModulationAndSpeed(int flags, boolean[] fm)
    {
        switch (flags)
        {
            case 0: /* 500 kbps FM */
                fm[0] = true;
                return 500;
            case 1: /* 300 kbps FM */
                fm[0] = true;
                return 300;
            case 2: /* 250 kbps FM */
                fm[0] = true;
                return 250;
            case 3: /* 500 kbps MFM */
                fm[0] = false;
                return 500;
            case 4: /* 300 kbps MFM */
                fm[0] = false;
                return 300;
            case 5: /* 250 kbps MFM */
                fm[0] = false;
                return 250;
            default:
                throw new FluxEngineException(
                        "IMD: don't understand IMD disks with this modulation and speed " + flags);
        }
    }

    private static int getSectorSize(int flags)
    {
        switch (flags)
        {
            case 0:
                return 128;
            case 1:
                return 256;
            case 2:
                return 512;
            case 3:
                return 1024;
            case 4:
                return 2048;
            case 5:
                return 4096;
            case 6:
                return 8192;
            default:
                throw new FluxEngineException("not reachable");
        }
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
            throw new FluxEngineException("IMD: cannot open input file");
        }
        int inputFileSize = data.size();
        ByteReader br = new ByteReader(data);
        Image image = new Image();
        int modeValue = 0;
        int track = 0;
        int head = 0;
        int numSectors = 0;
        int sectorSizeCode = 0;

        ConfigProto.Builder extra = ConfigProto.newBuilder();
        com.cowlark.fluxengine.config.LayoutProto.Builder layout = extra.getLayoutBuilder();

        int n = 0;
        int headerPtr = 0;
        int modulationSpeed = 0;
        int sectorSize = 0;
        List<Integer> sectorSkew = new ArrayList<>();

        /* Read comment */
        StringBuilder comment = new StringBuilder();
        int b;
        while ((b = br.read8()) != -1 && b != END_OF_FILE)
        {
            comment.append((char) b);
            n++;
        }
        headerPtr = n; /* set pointer to after comment */
        Logger.logf("Comment in IMD file: " + comment);

        boolean[] fm = {false};
        int trackSectorSize = -1;

        for (; ; )
        {
            if (headerPtr >= inputFileSize - 1)
            {
                break;
            }
            /* first read header */
            modeValue = br.read8();
            headerPtr++;
            modulationSpeed = getModulationAndSpeed(modeValue, fm);
            track = br.read8();
            headerPtr++;
            head = br.read8();
            headerPtr++;
            numSectors = br.read8();
            headerPtr++;
            sectorSizeCode = br.read8();
            headerPtr++;
            sectorSize = getSectorSize(sectorSizeCode);

            boolean blnOptionalCylinderMap = false;
            boolean blnOptionalHeadMap = false;
            List<Integer> optionalsectorMap = new ArrayList<>();
            List<Integer> optionalheadMap = new ArrayList<>();

            /* The Sector Cylinder Map has one entry for each sector, and
             * contains the logical Cylinder ID for the corresponding sector in
             * the Sector Numbering Map. */
            if ((head & SEC_CYL_MAP_FLAG) != 0)
            {
                /* Read optional cylinder map */
                for (b = 0; b < numSectors; b++)
                {
                    optionalsectorMap.add(br.read8());
                    headerPtr++;
                }
                blnOptionalCylinderMap = true;
                head = head ^ SEC_CYL_MAP_FLAG;
            }

            /* Read optional sector head map */
            if ((head & SEC_HEAD_MAP_FLAG) != 0)
            {
                /* Read optional sector head map */
                for (b = 0; b < numSectors; b++)
                {
                    optionalheadMap.add(br.read8());
                    headerPtr++;
                }
                blnOptionalHeadMap = true;
                head = head ^ SEC_HEAD_MAP_FLAG;
            }

            /* read sector numbering map */
            sectorSkew.clear();
            boolean blnBase0 = false; /* check what first start number of the sector is */
            for (b = 0; b < numSectors; b++)
            {
                int t = br.read8();
                if (t == 0x00)
                    blnBase0 = true;
                if (blnBase0)
                {
                    t = t + 1;
                }
                sectorSkew.add(t);
                headerPtr++;
            }

            IbmEncoderProto.Builder ibm = extra.getEncoderBuilder().getIbmBuilder();
            IbmEncoderProto.TrackdataProto.Builder trackdata = ibm.addTrackdataBuilder();

            com.cowlark.fluxengine.config.LayoutProto.LayoutdataProto.Builder layoutdata =
                    layout.addLayoutdataBuilder();

            trackdata.setTargetClockPeriodUs(1e3 / modulationSpeed);
            trackdata.setTargetRotationalPeriodMs(200);
            if (trackSectorSize < 0)
            {
                trackSectorSize = sectorSize;
                /* this is the first sector we've read, use its settings for
                 * per-track data */
                trackdata.setTrack(track);
                trackdata.setHead(head);
                trackdata.setUseFm(fm[0]);

                layoutdata.setTrack(track);
                layoutdata.setSide(head);
                layoutdata.setSectorSize(sectorSize);
            } else if (trackSectorSize != sectorSize)
            {
                throw new FluxEngineException(
                        "IMD: multiple sector sizes per track are currently unsupported");
            }

            /* read the sectors */
            for (int s = 0; s < numSectors; s++)
            {
                Bytes sectordata = new Bytes(0);
                Bytes compressed = new Bytes(sectorSize);
                int sectorId = sectorSkew.get(s);
                Sector sector = image.put(track, head, sectorId);
                /* read the status of the sector */
                int statusSector = br.read8();
                headerPtr++;

                switch (statusSector)
                {
                    case 0: /* Sector data unavailable - could not be read */
                        sector.status = Sector.Status.MISSING;
                        break;

                    case 1: /* Normal data: (Sector Size) bytes follow */
                        sectordata = br.read(sectorSize);
                        headerPtr += sectorSize;
                        sector.data = sectordata;
                        sector.status = Sector.Status.OK;
                        break;

                    case 2: /* Compressed: All bytes in sector have same value (xx) */
                        compressed.setByte(0, (byte) br.read8());
                        headerPtr++;
                        for (int k = 1; k < sectorSize; k++)
                        {
                            br.seek(headerPtr);
                            compressed.setByte(k, (byte) br.read8());
                        }
                        sector.data = compressed;
                        sector.status = Sector.Status.OK;
                        break;

                    case 3: /* Normal data with "Deleted-Data address mark" */
                        sector.status = Sector.Status.DATA_MISSING;
                        sectordata = br.read(sectorSize);
                        headerPtr += sectorSize;
                        sector.data = sectordata;
                        break;

                    case 4: /* Compressed with "Deleted-Data address mark" */
                        compressed.setByte(0, (byte) br.read8());
                        headerPtr++;
                        for (int k = 1; k < sectorSize; k++)
                        {
                            br.seek(headerPtr);
                            compressed.setByte(k, (byte) br.read8());
                        }
                        sector.data = compressed;
                        sector.status = Sector.Status.DATA_MISSING;
                        break;

                    case 5: /* Normal data read with data error */
                        sectordata = br.read(sectorSize);
                        headerPtr += sectorSize;
                        sector.status = Sector.Status.BAD_CHECKSUM;
                        sector.data = sectordata;
                        break;

                    case 6: /* Compressed read with data error */
                        compressed.setByte(0, (byte) br.read8());
                        headerPtr++;
                        for (int k = 1; k < sectorSize; k++)
                        {
                            br.seek(headerPtr);
                            compressed.setByte(k, (byte) br.read8());
                        }
                        sector.data = compressed;
                        sector.status = Sector.Status.BAD_CHECKSUM;
                        break;

                    case 7: /* Deleted data read with data error */
                        sectordata = br.read(sectorSize);
                        headerPtr += sectorSize;
                        sector.status = Sector.Status.BAD_CHECKSUM;
                        sector.data = sectordata;
                        break;

                    case 8: /* Compressed, Deleted read with data error */
                        compressed.setByte(0, (byte) br.read8());
                        headerPtr++;
                        for (int k = 1; k < sectorSize; k++)
                        {
                            br.seek(headerPtr);
                            compressed.setByte(k, (byte) br.read8());
                        }
                        sector.data = compressed;
                        sector.status = Sector.Status.BAD_CHECKSUM;
                        break;

                    default:
                        throw new FluxEngineException(String.format(
                                "IMD: Don't understand IMD files with sector status %d, " +
                                        "track %d, sector %d", statusSector, track, s));
                }

                if (blnOptionalCylinderMap)
                {
                    sector.logicalLocation = new CylinderHeadSector(
                            optionalsectorMap.get(s),
                            sector.logicalLocation.head(),
                            sector.logicalLocation.sector());
                    blnOptionalCylinderMap = false;
                } else
                    sector.logicalLocation = new CylinderHeadSector(
                            track,
                            sector.logicalLocation.head(),
                            sector.logicalLocation.sector());

                if (blnOptionalHeadMap)
                {
                    sector.logicalLocation =
                            new CylinderHeadSector(
                                    sector.logicalLocation.cylinder(),
                                    optionalheadMap.get(s),
                                    sector.logicalLocation.sector());
                    blnOptionalHeadMap = false;
                } else
                    sector.logicalLocation =
                            new CylinderHeadSector(
                                    sector.logicalLocation.cylinder(),
                                    head,
                                    sector.logicalLocation.sector());
            }
        }

        if (extra.getEncoder().getFormatCase() != EncoderProto.FormatCase.FORMAT_NOT_SET)
            Logger.logf("IMD: overriding configured format");

        image.calculateSize();
        Geometry geometry = image.getGeometry();
        int headSize = numSectors * sectorSize;
        int trackSize = headSize * (head + 1);

        Logger.logf("IMD: read " + (track + 1) + " tracks, " + (head + 1) + " heads; " +
                (fm[0] ? "FM" : "MFM") + "; " + modulationSpeed + " kbps; " + numSectors +
                " sectors; sectorsize " + sectorSize + "; " + (track + 1) * trackSize / 1024 +
                " kB total.");

        layout.setTracks(geometry.numCylinders);
        layout.setSides(geometry.numHeads);

        extraConfig = extra.build();
        return image;
    }
}
