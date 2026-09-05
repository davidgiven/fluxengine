package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.ArrayList;
import java.util.List;

@RunWith(JUnit4.class)
public class ReadWriteFluxOperationTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private static LogicalTrackLayout makeLtl()
    {
        ImmutableList<Integer> order = ImmutableList.of(0, 1, 2);
        return new LogicalTrackLayout(
                0,
                0,
                1,
                0,
                0,
                3,
                256,
                order,
                order,
                order,
                ImmutableMap.of(0, 0, 1, 1, 2, 2),
                ImmutableMap.of(0, 0, 1, 1, 2, 2));
    }

    private static Sector makeSector(int sectorId, Sector.Status status)
    {
        Sector sector = new Sector(new CylinderHeadSector(0, 0, sectorId));
        sector.status = status;
        return sector;
    }

    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("layout.tracks", "1")
                .set("layout.sides", "1")
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "8")
                .build();
    }

    private static ConfigProto makeConfigWithLayout(int tracks, int sides, String tracksOverride)
    {
        ConfigBuilder builder = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("layout.tracks", String.valueOf(tracks))
                .set("layout.sides", String.valueOf(sides))
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "4");
        if (tracksOverride != null)
            builder.set("tracks", tracksOverride);
        return builder.build();
    }

    private static ConfigProto makeConfigWithGroupSize(
            int tracks,
            int sides,
            String formatType,
            String driveType,
            String tracksOverride)
    {
        ConfigBuilder builder = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("drive.drive_type", driveType)
                .set("layout.tracks", String.valueOf(tracks))
                .set("layout.sides", String.valueOf(sides))
                .set("layout.format_type", formatType)
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "4");
        if (tracksOverride != null)
            builder.set("tracks", tracksOverride);
        return builder.build();
    }

    @Test
    public void collectSectorsDeduplicatesOkAndBad()
    {
        List<Sector> sectors = new ArrayList<>();
        sectors.add(makeSector(0, Sector.Status.OK));
        sectors.add(makeSector(0, Sector.Status.BAD_CHECKSUM));
        sectors.add(makeSector(1, Sector.Status.BAD_CHECKSUM));
        sectors.add(makeSector(1, Sector.Status.OK));
        sectors.add(makeSector(2, Sector.Status.BAD_CHECKSUM));

        List<Sector> result = ReadWriteFluxOperation.collectSectors(sectors, true);

        assertThat(result).hasSize(3);
        assertThat(result.get(0).status).isEqualTo(Sector.Status.OK);
        assertThat(result.get(1).status).isEqualTo(Sector.Status.OK);
        assertThat(result.get(2).status).isEqualTo(Sector.Status.BAD_CHECKSUM);
    }

    @Test
    public void collectSectorsPrefersOkOverMissing()
    {
        List<Sector> sectors = new ArrayList<>();
        sectors.add(makeSector(0, Sector.Status.MISSING));
        sectors.add(makeSector(0, Sector.Status.OK));

        List<Sector> result = ReadWriteFluxOperation.collectSectors(sectors);

        assertThat(result).hasSize(1);
        assertThat(result.get(0).status).isEqualTo(Sector.Status.OK);
    }

    @Test
    public void collectSectorsConflictWhenBothOkDifferentData()
    {
        Sector a = makeSector(0, Sector.Status.OK);
        a.data = Bytes.of(1);
        Sector b = makeSector(0, Sector.Status.OK);
        b.data = Bytes.of(2);

        /* collapseConflicts=false keeps both as CONFLICT. */
        List<Sector> result = ReadWriteFluxOperation.collectSectors(List.of(a, b), false);
        assertThat(result).hasSize(2);
        assertThat(result.get(0).status).isEqualTo(Sector.Status.CONFLICT);
        assertThat(result.get(1).status).isEqualTo(Sector.Status.CONFLICT);

        /* collapseConflicts=true collapses to a single CONFLICT. */
        List<Sector> collapsed = ReadWriteFluxOperation.collectSectors(List.of(a, b), true);
        assertThat(collapsed).hasSize(1);
        assertThat(collapsed.get(0).status).isEqualTo(Sector.Status.CONFLICT);
    }

    @Test
    public void collectSectorsOkDataSameCollapses()
    {
        Sector a = makeSector(0, Sector.Status.OK);
        a.data = Bytes.of(1);
        Sector b = makeSector(0, Sector.Status.OK);
        b.data = Bytes.of(1);

        List<Sector> result = ReadWriteFluxOperation.collectSectors(List.of(a, b), false);

        assertThat(result).hasSize(1);
        assertThat(result.get(0).status).isEqualTo(Sector.Status.OK);
    }

    @Test
    public void combineRecordAndSectorsFillsMissing()
    {
        /* A track with only sector 0 present; the layout wants 0,1,2. */
        Track track = new Track();
        track.allSectors = new ArrayList<>();
        track.allSectors.add(makeSector(0, Sector.Status.OK));

        ReadWriteFluxOperation.CombinationResult cr =
                ReadWriteFluxOperation.combineRecordAndSectors(List.of(track), makeLtl());

        assertThat(cr.result).isEqualTo(ReadWriteFluxOperation.BadSectorsState.HAS_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);

        Sector s0 =
                cr.sectors.stream().filter(s -> s.logicalLocation.sector() == 0).findFirst().get();
        Sector s1 =
                cr.sectors.stream().filter(s -> s.logicalLocation.sector() == 1).findFirst().get();
        Sector s2 =
                cr.sectors.stream().filter(s -> s.logicalLocation.sector() == 2).findFirst().get();
        assertThat(s0.status).isEqualTo(Sector.Status.OK);
        assertThat(s1.status).isEqualTo(Sector.Status.MISSING);
        assertThat(s2.status).isEqualTo(Sector.Status.MISSING);
    }

    @Test
    public void combineRecordAndSectorsNoBadWhenAllPresent()
    {
        Track track = new Track();
        track.allSectors = new ArrayList<>();
        track.allSectors.add(makeSector(0, Sector.Status.OK));
        track.allSectors.add(makeSector(1, Sector.Status.OK));
        track.allSectors.add(makeSector(2, Sector.Status.OK));

        ReadWriteFluxOperation.CombinationResult cr =
                ReadWriteFluxOperation.combineRecordAndSectors(List.of(track), makeLtl());

        assertThat(cr.result).isEqualTo(ReadWriteFluxOperation.BadSectorsState.HAS_NO_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);
        for (Sector sector : cr.sectors)
            assertThat(sector.status).isEqualTo(Sector.Status.OK);
    }

    @Test
    public void combineRecordAndSectorsEmptyTrackIsBad()
    {
        ReadWriteFluxOperation.CombinationResult cr =
                ReadWriteFluxOperation.combineRecordAndSectors(List.of(), makeLtl());

        assertThat(cr.result).isEqualTo(ReadWriteFluxOperation.BadSectorsState.HAS_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);
        for (Sector sector : cr.sectors)
            assertThat(sector.status).isEqualTo(Sector.Status.MISSING);
    }

    @Test
    public void getConfigReturnsConfiguredConfig()
    {
        ConfigProto config = makeConfig();
        TestOperation operation = new TestOperation();
        operation.setConfig(config);

        assertThat(operation.getConfig()).isSameInstanceAs(config);
    }

    @Test
    public void getDiskLayoutBuildsFromConfig()
    {
        TestOperation operation = new TestOperation();
        operation.setConfig(makeConfig());
        operation.init();

        DiskLayout diskLayout = operation.getDiskLayout();

        assertThat(diskLayout).isNotNull();
        assertThat(diskLayout.logicalLocations).isNotEmpty();
        assertThat(diskLayout.layoutByLogicalLocation.size()).isEqualTo(1);
    }

    /* ---------- computeLogicalLocations ---------- */

    @Test
    public void getDiskLayoutIsMemoized()
    {
        TestOperation operation = new TestOperation();
        operation.setConfig(makeConfig());
        operation.init();

        assertThat(operation.getDiskLayout()).isSameInstanceAs(operation.getDiskLayout());
    }

    @Test
    public void disposeDoesNotThrowWhenNothingCreated()
    {
        TestOperation operation = new TestOperation();
        operation.setConfig(makeConfig());
        operation.init();

        operation.dispose();
    }

    @Test
    public void computeLogicalLocationsReturnsAllWhenTracksNotSet()
    {
        ConfigProto config = makeConfigWithLayout(2, 2, null);
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        ImmutableList<CylinderHead> logical = op.computeLogicalLocations();

        assertThat(logical).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(0, 1),
                new CylinderHead(1, 0),
                new CylinderHead(1, 1));
        assertThat(logical).isEqualTo(op.getDiskLayout().logicalLocations);
    }

    @Test
    public void computeLogicalLocationsReturnsAllWhenEmptyString()
    {
        ConfigProto config = makeConfigWithLayout(1, 1, "");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        ImmutableList<CylinderHead> logical = op.computeLogicalLocations();

        assertThat(logical).containsExactly(new CylinderHead(0, 0));
        assertThat(logical).isEqualTo(op.getDiskLayout().logicalLocations);
    }

    @Test
    public void computeLogicalLocationsSingleTrack()
    {
        ConfigProto config = makeConfigWithLayout(2, 2, "c1h1");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.computeLogicalLocations()).containsExactly(new CylinderHead(1, 1));
    }

    @Test
    public void computeLogicalLocationsMultipleSorted()
    {
        /* Input unsorted: Locations parser sorts. */
        ConfigProto config = makeConfigWithLayout(2, 2, "c1h1 c0h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.computeLogicalLocations())
                .containsExactly(new CylinderHead(0, 0), new CylinderHead(1, 1))
                .inOrder();
    }

    @Test
    public void computeLogicalLocationsRangeAndStep()
    {
        ConfigProto config = makeConfigWithLayout(3, 2, "c0-2h0-1x2");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        /* Expands to c0,1,2 cross h0 ; h1 filtered by step 2 -> only h0 */
        assertThat(op.computeLogicalLocations())
                .containsExactly(
                        new CylinderHead(0, 0),
                        new CylinderHead(1, 0),
                        new CylinderHead(2, 0))
                .inOrder();
    }

    @Test
    public void computeLogicalLocationsCrossProduct()
    {
        ConfigProto config = makeConfigWithLayout(2, 2, "c0-1h0-1");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.computeLogicalLocations()).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(0, 1),
                new CylinderHead(1, 0),
                new CylinderHead(1, 1));
    }

    /* ---------- computePhysicalLocations ---------- */

    @Test
    public void computePhysicalLocationsGroupSizeOneSingle()
    {
        ConfigProto config = makeConfigWithLayout(1, 1, null);
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        ImmutableList<CylinderHead> physical = op.computePhysicalLocations();

        assertThat(physical).containsExactly(new CylinderHead(0, 0));
    }

    @Test
    public void computePhysicalLocationsGroupSizeOneMultiple()
    {
        ConfigProto config = makeConfigWithLayout(2, 2, null);
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        ImmutableList<CylinderHead> physical = op.computePhysicalLocations();

        /* With groupSize 1, physical == logical */
        assertThat(physical).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(0, 1),
                new CylinderHead(1, 0),
                new CylinderHead(1, 1));
    }

    @Test
    public void computePhysicalLocationsWithOverrideSingleLogicalGroupSizeOne()
    {
        ConfigProto config = makeConfigWithLayout(2, 2, "c1h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.computePhysicalLocations()).containsExactly(new CylinderHead(1, 0));
    }

    @Test
    public void computePhysicalLocationsExpandsGroupSizeTwoSingleLogical()
    {
        ConfigProto config =
                makeConfigWithGroupSize(2, 1, "FORMATTYPE_40TRACK", "DRIVETYPE_80TRACK", null);
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.getDiskLayout().groupSize).isEqualTo(2);

        /* Logical cylinders 0 and 1 map to physical 0,1 and 2,3 respectively. */
        ImmutableList<CylinderHead> physical = op.computePhysicalLocations();

        assertThat(physical).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(1, 0),
                new CylinderHead(2, 0),
                new CylinderHead(3, 0)).inOrder();
    }

    @Test
    public void computePhysicalLocationsExpandsGroupSizeTwoWithOverride()
    {
        ConfigProto config =
                makeConfigWithGroupSize(2, 1, "FORMATTYPE_40TRACK", "DRIVETYPE_80TRACK", "c1h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.getDiskLayout().groupSize).isEqualTo(2);

        /* Logical c1 maps to physical 2 and 3 (head 0). */
        assertThat(op.computePhysicalLocations())
                .containsExactly(new CylinderHead(2, 0), new CylinderHead(3, 0))
                .inOrder();
    }

    @Test
    public void computePhysicalLocationsGroupSizeTwoMultipleLogicalWithTwoSides()
    {
        ConfigProto config = makeConfigWithGroupSize(
                2,
                2,
                "FORMATTYPE_40TRACK",
                "DRIVETYPE_80TRACK",
                "c0-1h0-1");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.getDiskLayout().groupSize).isEqualTo(2);

        /* Order follows logicalLocations sorted order: (0,0) -> 0,1 ; (0,1) -> 0,1 on head1 etc?
         Actually physicalHead = logicalHead remapped.
           With swapSides false and headBias 0, physicalHead == logicalHead.
           So (0,0)-> p0h0,p1h0 ; (0,1)-> p0h1,p1h1 ; (1,0)-> p2h0,p3h0 ; (1,1)-> p2h1,p3h1
        */
        assertThat(op.computePhysicalLocations()).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(1, 0),
                new CylinderHead(0, 1),
                new CylinderHead(1, 1),
                new CylinderHead(2, 0),
                new CylinderHead(3, 0),
                new CylinderHead(2, 1),
                new CylinderHead(3, 1)).inOrder();
    }

    @Test
    public void computePhysicalLocationsFilteredSubsetGroupSizeTwo()
    {
        ConfigProto config = makeConfigWithGroupSize(
                3,
                1,
                "FORMATTYPE_40TRACK",
                "DRIVETYPE_80TRACK",
                "c0h0 c2h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThat(op.getDiskLayout().groupSize).isEqualTo(2);

        /* c0 -> 0,1 ; c2 -> 4,5 */
        assertThat(op.computePhysicalLocations()).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(1, 0),
                new CylinderHead(4, 0),
                new CylinderHead(5, 0)).inOrder();
    }

    @Test
    public void computePhysicalLocationsPreservesSortedLogicalOrder()
    {
        /* Input logical order unsorted but parser sorts; physical should follow sorted. */
        ConfigProto config = makeConfigWithGroupSize(
                2,
                1,
                "FORMATTYPE_40TRACK",
                "DRIVETYPE_80TRACK",
                "c1h0 c0h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        ImmutableList<CylinderHead> physical = op.computePhysicalLocations();

        /* Sorted logical is c0h0 then c1h0 => physical 0,1 then 2,3 */
        assertThat(physical).containsExactly(
                new CylinderHead(0, 0),
                new CylinderHead(1, 0),
                new CylinderHead(2, 0),
                new CylinderHead(3, 0)).inOrder();
    }

    @Test
    public void computePhysicalLocationsThrowsWhenLogicalNotInLayout()
    {
        ConfigProto config = makeConfigWithLayout(1, 1, "c5h0");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        assertThrows(FluxEngineException.class, () -> op.computePhysicalLocations());
    }

    @Test
    public void computePhysicalLocationsThrowsMessageContainsNotPartOfFormat()
    {
        ConfigProto config = makeConfigWithLayout(1, 1, "c0h5");
        TestOperation op = new TestOperation();
        op.setConfig(config);
        op.init();

        FluxEngineException ex =
                assertThrows(FluxEngineException.class, () -> op.computePhysicalLocations());
        assertThat(ex.getMessage()).contains("isn't part of the format");
    }

    private static class TestOperation extends ReadWriteFluxOperation
    {
        @Override
        public void run()
        {
        }
    }
}
