package com.cowlark.fluxengine.data;

import static com.cowlark.fluxengine.external.DriveType.DRIVETYPE_80TRACK;
import static com.cowlark.fluxengine.external.FormatType.FORMATTYPE_40TRACK;
import static com.cowlark.fluxengine.external.FormatType.FORMATTYPE_80TRACK;
import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.DriveProto;
import com.cowlark.fluxengine.config.LayoutProto;
import com.google.common.collect.ImmutableMap;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.function.Consumer;

@RunWith(JUnit4.class)
public class DiskLayoutTest
{
    private static DiskLayout diskLayout(
            com.cowlark.fluxengine.external.FormatType formatType,
            Consumer<LayoutProto.LayoutdataProto.Builder> layoutData)
    {
        ConfigProto.Builder config = baseConfig(formatType);
        config.getLayoutBuilder().setTracks(78).setSides(2);
        layoutData.accept(addLayoutData(config));
        return new DiskLayout(config.build());
    }

    private static LogicalTrackLayout logicalLayoutAt(DiskLayout diskLayout, int cylinder, int head)
    {
        return diskLayout.layoutByPhysicalLocation.get(new CylinderHead(
                cylinder,
                head)).logicalTrackLayout;
    }

    private static ConfigProto.Builder baseConfig(com.cowlark.fluxengine.external.FormatType formatType)
    {
        return ConfigProto
                .newBuilder()
                .setDrive(DriveProto.newBuilder().setDriveType(DRIVETYPE_80TRACK).build())
                .setLayout(LayoutProto.newBuilder().setFormatType(formatType).build());
    }

    private static LayoutProto.LayoutdataProto.Builder addLayoutData(ConfigProto.Builder config)
    {
        return config.getLayoutBuilder().addLayoutdataBuilder();
    }

    @Test
    public void testPhysicalSectors()
    {
        DiskLayout diskLayout = diskLayout(
                FORMATTYPE_80TRACK, (track) -> {
                    track.setSectorSize(256);
                    track.getPhysicalBuilder().addSector(0).addSector(2).addSector(1).addSector(3);
                });

        LogicalTrackLayout layout = logicalLayoutAt(diskLayout, 0, 0);
        assertThat(diskLayout.layoutByLogicalLocation.get(new CylinderHead(0, 0))).isSameInstanceAs(
                layout);
        assertThat(layout.naturalSectorOrder).containsExactly(0, 1, 2, 3).inOrder();
        assertThat(layout.diskSectorOrder).containsExactly(0, 2, 1, 3).inOrder();
        assertThat(layout.filesystemSectorOrder).containsExactly(0, 1, 2, 3).inOrder();
    }

    @Test
    public void testLogicalSectors()
    {
        DiskLayout diskLayout = diskLayout(
                FORMATTYPE_80TRACK, (track) -> {
                    track.setSectorSize(256);
                    track.getPhysicalBuilder().addSector(0).addSector(1).addSector(2).addSector(3);
                    track
                            .getFilesystemBuilder()
                            .addSector(0)
                            .addSector(2)
                            .addSector(1)
                            .addSector(3);
                });

        LogicalTrackLayout layout = logicalLayoutAt(diskLayout, 0, 0);
        assertThat(diskLayout.layoutByLogicalLocation.get(new CylinderHead(0, 0))).isSameInstanceAs(
                layout);
        assertThat(layout.naturalSectorOrder).containsExactly(0, 1, 2, 3).inOrder();
        assertThat(layout.diskSectorOrder).containsExactly(0, 1, 2, 3).inOrder();
        assertThat(layout.filesystemSectorOrder).containsExactly(0, 2, 1, 3).inOrder();
    }

    @Test
    public void test_bothSectors()
    {
        DiskLayout diskLayout = diskLayout(
                FORMATTYPE_80TRACK, (track) -> {
                    track.setSectorSize(256);
                    track.getPhysicalBuilder().addSector(3).addSector(2).addSector(1).addSector(0);
                    track
                            .getFilesystemBuilder()
                            .addSector(0)
                            .addSector(2)
                            .addSector(1)
                            .addSector(3);
                });

        LogicalTrackLayout layout = logicalLayoutAt(diskLayout, 0, 0);
        assertThat(diskLayout.layoutByLogicalLocation.get(new CylinderHead(0, 0))).isSameInstanceAs(
                layout);
        assertThat(layout.naturalSectorOrder).containsExactly(0, 1, 2, 3).inOrder();
        assertThat(layout.diskSectorOrder).containsExactly(3, 2, 1, 0).inOrder();
        assertThat(layout.filesystemSectorOrder).containsExactly(0, 2, 1, 3).inOrder();
    }

    @Test
    public void test_skew()
    {
        DiskLayout diskLayout = diskLayout(
                FORMATTYPE_80TRACK, (track) -> {
                    track.setSectorSize(256);
                    track.getPhysicalBuilder().setStartSector(0).setCount(12).setSkew(6);
                });

        LogicalTrackLayout layout = logicalLayoutAt(diskLayout, 0, 0);
        assertThat(layout.naturalSectorOrder)
                .containsExactly(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11)
                .inOrder();
        assertThat(layout.diskSectorOrder)
                .containsExactly(0, 6, 1, 7, 2, 8, 3, 9, 4, 10, 5, 11)
                .inOrder();
    }

    @Test
    public void test_bounds()
    {
        ConfigProto.Builder config = baseConfig(FORMATTYPE_40TRACK);
        config.getLayoutBuilder().setTracks(2).setSides(2);
        addLayoutData(config)
                .setSectorSize(256)
                .getPhysicalBuilder()
                .setStartSector(0)
                .setCount(12)
                .setSkew(6);

        DiskLayout diskLayout = new DiskLayout(config.build());
        assertThat(diskLayout.groupSize).isEqualTo(2);
        assertThat(diskLayout.getLogicalBounds()).isEqualTo(new DiskLayout.LayoutBounds(
                0,
                1,
                0,
                1));
        assertThat(diskLayout.getPhysicalBounds()).isEqualTo(new DiskLayout.LayoutBounds(
                0,
                3,
                0,
                1));
    }

    @Test
    public void test_sectoroffsets()
    {
        ConfigProto.Builder config = baseConfig(FORMATTYPE_80TRACK);
        config.getLayoutBuilder().setTracks(2).setSides(2);
        LayoutProto.LayoutdataProto.Builder layoutData = addLayoutData(config);
        layoutData.setSectorSize(256);
        layoutData.getPhysicalBuilder().setStartSector(0).setCount(4);
        layoutData.getFilesystemBuilder().setStartSector(0).setCount(4).setSkew(2);

        DiskLayout diskLayout = new DiskLayout(config.build());
        assertThat(diskLayout.groupSize).isEqualTo(1);
        assertThat(diskLayout.logicalSectorLocationBySectorOffset).isEqualTo(ImmutableMap
                .<Long, CylinderHeadSector>builder()
                .put(0L, new CylinderHeadSector(0, 0, 0))
                .put(256L, new CylinderHeadSector(0, 0, 2))
                .put(512L, new CylinderHeadSector(0, 0, 1))
                .put(768L, new CylinderHeadSector(0, 0, 3))
                .put(1024L, new CylinderHeadSector(0, 1, 0))
                .put(1280L, new CylinderHeadSector(0, 1, 2))
                .put(1536L, new CylinderHeadSector(0, 1, 1))
                .put(1792L, new CylinderHeadSector(0, 1, 3))
                .put(2048L, new CylinderHeadSector(1, 0, 0))
                .put(2304L, new CylinderHeadSector(1, 0, 2))
                .put(2560L, new CylinderHeadSector(1, 0, 1))
                .put(2816L, new CylinderHeadSector(1, 0, 3))
                .put(3072L, new CylinderHeadSector(1, 1, 0))
                .put(3328L, new CylinderHeadSector(1, 1, 2))
                .put(3584L, new CylinderHeadSector(1, 1, 1))
                .put(3840L, new CylinderHeadSector(1, 1, 3))
                .build());
        assertThat(diskLayout.sectorOffsetByLogicalSectorLocation).isEqualTo(ImmutableMap
                .<CylinderHeadSector, Long>builder()
                .put(new CylinderHeadSector(0, 0, 0), 0L)
                .put(new CylinderHeadSector(0, 0, 1), 512L)
                .put(new CylinderHeadSector(0, 0, 2), 256L)
                .put(new CylinderHeadSector(0, 0, 3), 768L)
                .put(new CylinderHeadSector(0, 1, 0), 1024L)
                .put(new CylinderHeadSector(0, 1, 1), 1536L)
                .put(new CylinderHeadSector(0, 1, 2), 1280L)
                .put(new CylinderHeadSector(0, 1, 3), 1792L)
                .put(new CylinderHeadSector(1, 0, 0), 2048L)
                .put(new CylinderHeadSector(1, 0, 1), 2560L)
                .put(new CylinderHeadSector(1, 0, 2), 2304L)
                .put(new CylinderHeadSector(1, 0, 3), 2816L)
                .put(new CylinderHeadSector(1, 1, 0), 3072L)
                .put(new CylinderHeadSector(1, 1, 1), 3584L)
                .put(new CylinderHeadSector(1, 1, 2), 3328L)
                .put(new CylinderHeadSector(1, 1, 3), 3840L)
                .build());
    }

    @Test
    public void test_equality()
    {
        ConfigProto.Builder config1 = baseConfig(FORMATTYPE_80TRACK);
        config1.getLayoutBuilder().setTracks(2).setSides(2);
        addLayoutData(config1)
                .setSectorSize(256)
                .getPhysicalBuilder()
                .setStartSector(0)
                .setCount(4);

        ConfigProto.Builder config2 = baseConfig(FORMATTYPE_80TRACK);
        config2.getLayoutBuilder().setTracks(2).setSides(2);
        addLayoutData(config2)
                .setSectorSize(256)
                .getPhysicalBuilder()
                .setStartSector(0)
                .setCount(4);

        DiskLayout layout1 = new DiskLayout(config1.build());
        DiskLayout layout2 = new DiskLayout(config2.build());

        assertThat(layout1).isEqualTo(layout2);
        assertThat(layout1.hashCode()).isEqualTo(layout2.hashCode());
    }

    @Test
    public void test_inequality()
    {
        ConfigProto.Builder config1 = baseConfig(FORMATTYPE_80TRACK);
        config1.getLayoutBuilder().setTracks(2).setSides(2);
        addLayoutData(config1)
                .setSectorSize(256)
                .getPhysicalBuilder()
                .setStartSector(0)
                .setCount(4);

        ConfigProto.Builder config2 = baseConfig(FORMATTYPE_80TRACK);
        config2.getLayoutBuilder().setTracks(2).setSides(2);
        addLayoutData(config2)
                .setSectorSize(512)
                .getPhysicalBuilder()
                .setStartSector(0)
                .setCount(4);

        DiskLayout layout1 = new DiskLayout(config1.build());
        DiskLayout layout2 = new DiskLayout(config2.build());

        assertThat(layout1).isNotEqualTo(layout2);
    }
}