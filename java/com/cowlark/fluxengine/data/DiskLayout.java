package com.cowlark.fluxengine.data;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.LayoutProto;
import com.cowlark.fluxengine.config.SectorListProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.external.DriveType;
import com.cowlark.fluxengine.external.FormatType;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import java.util.ArrayList;
import java.util.Collections;
import java.util.HashSet;
import java.util.LinkedHashMap;
import java.util.List;
import java.util.Map;
import java.util.Set;

/**
 * The physical layout of the disk, ported from lib/data/layout.cc.
 */
public class DiskLayout
{
    /* Logical size. */
    public final int numLogicalCylinders;
    public final int numLogicalHeads;
    /* Physical size and properties. */
    public final int minPhysicalCylinder;
    public final int maxPhysicalCylinder;
    public final int minPhysicalHead;
    public final int maxPhysicalHead;
    public final int groupSize;
    public final int headBias;
    public final int headWidth;
    public final boolean swapSides;
    public final long totalBytes;
    /* Physical and logical layouts by location. */
    public final ImmutableMap<CylinderHead, PhysicalTrackLayout> layoutByPhysicalLocation;
    public final ImmutableMap<CylinderHead, LogicalTrackLayout> layoutByLogicalLocation;
    /* Ordered lists of physical and logical locations. */
    public final ImmutableList<CylinderHead> logicalLocations;
    public final ImmutableList<CylinderHead> logicalLocationsInFilesystemOrder;
    public final ImmutableList<CylinderHead> physicalLocations;
    /* Ordered lists of sector locations, plus the reverse mapping. */
    public final ImmutableList<LogicalLocation> logicalSectorLocationsInFilesystemOrder;
    public final ImmutableMap<LogicalLocation, Integer> blockIdByLogicalSectorLocation;
    public final ImmutableList<CylinderHeadSector> physicalSectorLocationsInFilesystemOrder;
    /* Mapping from logical location to sector offset and back again. */
    public final ImmutableMap<Long, LogicalLocation> logicalSectorLocationBySectorOffset;
    public final ImmutableMap<LogicalLocation, Long> sectorOffsetByLogicalSectorLocation;

    public DiskLayout(ConfigProto config)
    {
        int minPhysicalCylinderLocal = Integer.MAX_VALUE;
        int minPhysicalHeadLocal = Integer.MAX_VALUE;
        int maxPhysicalCylinderLocal = 0;
        int maxPhysicalHeadLocal = 0;

        numLogicalCylinders = config.getLayout().getTracks();
        numLogicalHeads = config.getLayout().getSides();

        groupSize = getTrackStep(config);
        headBias = config.getDrive().getHeadBias();
        swapSides = config.getLayout().getSwapSides();

        switch (config.getDrive().getDriveType())
        {
            case DRIVETYPE_APPLE2:
                headWidth = 4;
                break;

            default:
                headWidth = 1;
                break;
        }

        Map<CylinderHead, LogicalTrackLayout> logicalLayout = new LinkedHashMap<>();
        List<CylinderHead> logicalLocationsLocal = new ArrayList<>();

        for (int logicalCylinder = 0; logicalCylinder < numLogicalCylinders; logicalCylinder++)
            for (int logicalHead = 0; logicalHead < numLogicalHeads; logicalHead++)
            {
                int physicalCylinder = remapCylinderLogicalToPhysical(logicalCylinder);
                int physicalHead = remapHeadLogicalToPhysical(logicalHead);

                minPhysicalCylinderLocal = Math.min(minPhysicalCylinderLocal, physicalCylinder);
                maxPhysicalCylinderLocal =
                        Math.max(maxPhysicalCylinderLocal, physicalCylinder + groupSize - 1);
                minPhysicalHeadLocal = Math.min(minPhysicalHeadLocal, physicalHead);
                maxPhysicalHeadLocal = Math.max(maxPhysicalHeadLocal, physicalHead);

                LayoutProto.LayoutdataProto layoutdata =
                        getLayoutData(logicalCylinder, logicalHead, config);
                int sectorSize = layoutdata.getSectorSize();
                List<Integer> diskSectorOrder = expandSectorList(layoutdata.getPhysical());
                List<Integer> naturalSectorOrder = new ArrayList<>(diskSectorOrder);
                Collections.sort(naturalSectorOrder);
                int numSectors = naturalSectorOrder.size();

                List<Integer> filesystemSectorOrder;
                if (layoutdata.hasFilesystem())
                {
                    filesystemSectorOrder = expandSectorList(layoutdata.getFilesystem());
                    if (filesystemSectorOrder.size() != numSectors)
                        throw new FluxEngineException(
                                "filesystem sector order list doesn't contain the right number of" +
                                        " sectors");
                } else
                    filesystemSectorOrder = new ArrayList<>(naturalSectorOrder);

                Map<Integer, Integer> sectorIdToNaturalOrdering = new LinkedHashMap<>();
                Map<Integer, Integer> sectorIdToFilesystemOrdering = new LinkedHashMap<>();
                for (int i = 0; i < numSectors; i++)
                {
                    int fid = naturalSectorOrder.get(i);
                    sectorIdToNaturalOrdering.put(i, fid);
                    sectorIdToFilesystemOrdering.put(i, fid);
                }

                LogicalTrackLayout ltl = new LogicalTrackLayout(
                        physicalCylinder,
                        physicalHead,
                        groupSize,
                        logicalCylinder,
                        logicalHead,
                        numSectors,
                        sectorSize,
                        ImmutableList.copyOf(naturalSectorOrder),
                        ImmutableList.copyOf(diskSectorOrder),
                        ImmutableList.copyOf(filesystemSectorOrder),
                        ImmutableMap.copyOf(sectorIdToFilesystemOrdering),
                        ImmutableMap.copyOf(sectorIdToNaturalOrdering));
                logicalLayout.put(new CylinderHead(logicalCylinder, logicalHead), ltl);
                logicalLocationsLocal.add(new CylinderHead(logicalCylinder, logicalHead));
            }

        minPhysicalCylinder = minPhysicalCylinderLocal;
        maxPhysicalCylinder = maxPhysicalCylinderLocal;
        minPhysicalHead = minPhysicalHeadLocal;
        maxPhysicalHead = maxPhysicalHeadLocal;

        Map<CylinderHead, PhysicalTrackLayout> physicalLayout = new LinkedHashMap<>();
        List<CylinderHead> physicalLocationsLocal = new ArrayList<>();

        for (int physicalCylinder = minPhysicalCylinder; physicalCylinder <= maxPhysicalCylinder;
             physicalCylinder++)
            for (int physicalHead = minPhysicalHead; physicalHead <= maxPhysicalHead;
                 physicalHead++)
            {
                CylinderHead ch = new CylinderHead(physicalCylinder, physicalHead);
                PhysicalTrackLayout ptl = new PhysicalTrackLayout(
                        physicalCylinder,
                        physicalHead,
                        (physicalCylinder - headBias) % groupSize,
                        logicalLayout.get(new CylinderHead(
                                remapCylinderPhysicalToLogical(physicalCylinder),
                                remapHeadPhysicalToLogical(physicalHead))));
                physicalLayout.put(ch, ptl);
                physicalLocationsLocal.add(ch);
            }

        layoutByLogicalLocation = ImmutableMap.copyOf(logicalLayout);
        logicalLocations = ImmutableList.copyOf(logicalLocationsLocal);
        layoutByPhysicalLocation = ImmutableMap.copyOf(physicalLayout);
        physicalLocations = ImmutableList.copyOf(physicalLocationsLocal);

        long sectorOffset = 0;
        int blockId = 0;
        List<CylinderHead> logicalLocationsFilesystemLocal = new ArrayList<>();
        List<LogicalLocation> logicalSectorLocationsLocal = new ArrayList<>();
        Map<Long, LogicalLocation> logicalSectorOffsetLocal = new LinkedHashMap<>();
        Map<LogicalLocation, Long> sectorOffsetByLocationLocal = new LinkedHashMap<>();
        Map<LogicalLocation, Integer> blockIdByLocationLocal = new LinkedHashMap<>();

        for (CylinderHead ch : getTrackOrdering(
                config.getLayout().getFilesystemTrackOrder(),
                numLogicalCylinders,
                numLogicalHeads))
        {
            LogicalTrackLayout ltl = logicalLayout.get(ch);
            logicalLocationsFilesystemLocal.add(ch);

            for (int lid : ltl.filesystemSectorOrder)
            {
                LogicalLocation logicalLocation =
                        new LogicalLocation(ch.cylinder(), ch.head(), lid);
                logicalSectorOffsetLocal.put(sectorOffset, logicalLocation);
                sectorOffsetByLocationLocal.put(logicalLocation, sectorOffset);
                logicalSectorLocationsLocal.add(logicalLocation);
                sectorOffset += ltl.sectorSize;

                blockIdByLocationLocal.put(logicalLocation, blockId);
                blockId++;
            }
        }

        logicalLocationsInFilesystemOrder = ImmutableList.copyOf(logicalLocationsFilesystemLocal);
        logicalSectorLocationsInFilesystemOrder = ImmutableList.copyOf(logicalSectorLocationsLocal);
        logicalSectorLocationBySectorOffset = ImmutableMap.copyOf(logicalSectorOffsetLocal);
        sectorOffsetByLogicalSectorLocation = ImmutableMap.copyOf(sectorOffsetByLocationLocal);
        blockIdByLogicalSectorLocation = ImmutableMap.copyOf(blockIdByLocationLocal);
        physicalSectorLocationsInFilesystemOrder = ImmutableList.of();

        totalBytes = sectorOffset;
    }

    public DiskLayout(int numCylinders, int numHeads, int numSectors, int sectorSize)
    {
        this(createTestConfig(numCylinders, numHeads, numSectors, sectorSize));
    }

    public static DiskLayout createDiskLayout(ConfigProto config)
    {
        return new DiskLayout(config);
    }

    public static LayoutBounds getBounds(Iterable<CylinderHead> keys)
    {
        int minCylinder = Integer.MAX_VALUE;
        int maxCylinder = Integer.MIN_VALUE;
        int minHead = Integer.MAX_VALUE;
        int maxHead = Integer.MIN_VALUE;

        for (CylinderHead ch : keys)
        {
            minCylinder = Math.min(minCylinder, ch.cylinder());
            maxCylinder = Math.max(maxCylinder, ch.cylinder());
            minHead = Math.min(minHead, ch.head());
            maxHead = Math.max(maxHead, ch.head());
        }

        return new LayoutBounds(minCylinder, maxCylinder, minHead, maxHead);
    }

    private static int getTrackStep(ConfigProto config)
    {
        FormatType formatType = config.getLayout().getFormatType();
        DriveType driveType = config.getDrive().getDriveType();

        switch (formatType)
        {
            case FORMATTYPE_40TRACK:
                switch (driveType)
                {
                    case DRIVETYPE_40TRACK:
                        return 1;

                    case DRIVETYPE_80TRACK:
                        return 2;

                    case DRIVETYPE_APPLE2:
                        return 4;

                    default:
                        break;
                }

                /* Fall through, as in the C++. */

            case FORMATTYPE_80TRACK:
                switch (driveType)
                {
                    case DRIVETYPE_40TRACK:
                        throw new FluxEngineException(
                                "you can't read/write an 80 track image from/to a 40 track drive");

                    case DRIVETYPE_80TRACK:
                        return 1;

                    case DRIVETYPE_APPLE2:
                        throw new FluxEngineException(
                                "you can't read/write an 80 track image from/to an Apple II drive");

                    default:
                        break;
                }
                break;

            default:
                break;
        }

        return 1;
    }

    private static List<CylinderHead> getTrackOrdering(LayoutProto.Order ordering,
                                                       int tracks,
                                                       int sides)
    {
        List<CylinderHead> trackList = new ArrayList<>();
        switch (ordering)
        {
            case CHS:
                for (int track = 0; track < tracks; track++)
                    for (int side = 0; side < sides; side++)
                        trackList.add(new CylinderHead(track, side));
                break;

            case HCS:
                for (int side = 0; side < sides; side++)
                    for (int track = 0; track < tracks; track++)
                        trackList.add(new CylinderHead(track, side));
                break;

            case HCS_RH1:
                for (int side = 0; side < sides; side++)
                {
                    if (side == 0)
                        for (int track = 0; track < tracks; track++)
                            trackList.add(new CylinderHead(track, side));
                    if (side == 1)
                        for (int track = tracks; track > 0; track--)
                            trackList.add(new CylinderHead(track - 1, side));
                }
                break;

            default:
                throw new FluxEngineException("LAYOUT: invalid track trackList");
        }

        return trackList;
    }

    private static List<Integer> expandSectorList(SectorListProto sectorsProto)
    {
        List<Integer> sectors = new ArrayList<>();

        if (sectorsProto.hasCount())
        {
            if (sectorsProto.getSectorCount() != 0)
                throw new FluxEngineException(
                        "LAYOUT: if you use a sector count, you can't use an explicit sector list");

            Set<Integer> sectorset = new HashSet<>();
            int id = sectorsProto.getStartSector();
            for (int i = 0; i < sectorsProto.getCount(); i++)
            {
                while (sectorset.contains(id))
                {
                    id++;
                    if (id >= (sectorsProto.getStartSector() + sectorsProto.getCount()))
                        id -= sectorsProto.getCount();
                }

                sectorset.add(id);
                sectors.add(id);

                id += sectorsProto.getSkew();
                if (id >= (sectorsProto.getStartSector() + sectorsProto.getCount()))
                    id -= sectorsProto.getCount();
            }
        } else if (sectorsProto.getSectorCount() > 0)
        {
            for (int i = 0; i < sectorsProto.getSectorCount(); i++)
                sectors.add(sectorsProto.getSector(i));
        } else
            throw new FluxEngineException("LAYOUT: no sectors in sector definition!");

        return sectors;
    }

    private static LayoutProto.LayoutdataProto getLayoutData(int logicalCylinder,
                                                             int logicalHead,
                                                             ConfigProto config)
    {
        LayoutProto.LayoutdataProto.Builder layoutData = LayoutProto.LayoutdataProto.newBuilder();
        for (LayoutProto.LayoutdataProto f : config.getLayout().getLayoutdataList())
        {
            if (f.hasTrack() && f.hasUpToTrack() &&
                    ((logicalCylinder < f.getTrack()) || (logicalCylinder > f.getUpToTrack())))
                continue;
            if (f.hasTrack() && !f.hasUpToTrack() && (logicalCylinder != f.getTrack()))
                continue;
            if (f.hasSide() && (f.getSide() != logicalHead))
                continue;

            layoutData.mergeFrom(f);
        }
        return layoutData.build();
    }

    private static ConfigProto createTestConfig(int numCylinders,
                                                int numHeads,
                                                int numSectors,
                                                int sectorSize)
    {
        ConfigProto.Builder config = ConfigProto.newBuilder();
        LayoutProto.Builder layout = config.getLayoutBuilder();
        layout.setTracks(numCylinders);
        layout.setSides(numHeads);
        LayoutProto.LayoutdataProto.Builder layoutData = layout.addLayoutdataBuilder();
        layoutData.setSectorSize(sectorSize);
        layoutData.getPhysicalBuilder().setCount(numSectors);

        return config.build();
    }

    public LayoutBounds getPhysicalBounds()
    {
        return getBounds(layoutByPhysicalLocation.keySet());
    }

    public LayoutBounds getLogicalBounds()
    {
        return getBounds(layoutByLogicalLocation.keySet());
    }

    public int remapCylinderPhysicalToLogical(int physicalCylinder)
    {
        return (physicalCylinder - headBias) / groupSize;
    }

    public int remapCylinderLogicalToPhysical(int logicalCylinder)
    {
        return headBias + logicalCylinder * groupSize;
    }

    public int remapHeadPhysicalToLogical(int physicalHead)
    {
        return physicalHead ^ (swapSides ? 1 : 0);
    }

    public int remapHeadLogicalToPhysical(int logicalHead)
    {
        return logicalHead ^ (swapSides ? 1 : 0);
    }

    public record LayoutBounds(int minCylinder, int maxCylinder, int minHead, int maxHead)
    {
    }
}