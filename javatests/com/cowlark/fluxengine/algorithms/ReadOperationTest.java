package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.LogicalTrackLayout;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.data.Track;
import com.google.common.collect.ImmutableList;
import com.google.common.collect.ImmutableMap;
import java.util.ArrayList;
import java.util.List;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ReadOperationTest
{
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
        Sector sector = new Sector(new LogicalLocation(0, 0, sectorId));
        sector.status = status;
        return sector;
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

        List<Sector> result = ReadOperation.collectSectors(sectors, true);

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

        List<Sector> result = ReadOperation.collectSectors(sectors);

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
        List<Sector> result = ReadOperation.collectSectors(List.of(a, b), false);
        assertThat(result).hasSize(2);
        assertThat(result.get(0).status).isEqualTo(Sector.Status.CONFLICT);
        assertThat(result.get(1).status).isEqualTo(Sector.Status.CONFLICT);

        /* collapseConflicts=true collapses to a single CONFLICT. */
        List<Sector> collapsed = ReadOperation.collectSectors(List.of(a, b), true);
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

        List<Sector> result = ReadOperation.collectSectors(List.of(a, b), false);

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

        ReadOperation.CombinationResult cr =
                ReadOperation.combineRecordAndSectors(List.of(track), makeLtl());

        assertThat(cr.result).isEqualTo(ReadOperation.BadSectorsState.HAS_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);

        Sector s0 =
                cr.sectors.stream().filter(s -> s.location.logicalSector() == 0).findFirst().get();
        Sector s1 =
                cr.sectors.stream().filter(s -> s.location.logicalSector() == 1).findFirst().get();
        Sector s2 =
                cr.sectors.stream().filter(s -> s.location.logicalSector() == 2).findFirst().get();
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

        ReadOperation.CombinationResult cr =
                ReadOperation.combineRecordAndSectors(List.of(track), makeLtl());

        assertThat(cr.result).isEqualTo(ReadOperation.BadSectorsState.HAS_NO_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);
        for (Sector sector : cr.sectors)
            assertThat(sector.status).isEqualTo(Sector.Status.OK);
    }

    @Test
    public void combineRecordAndSectorsEmptyTrackIsBad()
    {
        ReadOperation.CombinationResult cr =
                ReadOperation.combineRecordAndSectors(List.of(), makeLtl());

        assertThat(cr.result).isEqualTo(ReadOperation.BadSectorsState.HAS_BAD_SECTORS);
        assertThat(cr.sectors).hasSize(3);
        for (Sector sector : cr.sectors)
            assertThat(sector.status).isEqualTo(Sector.Status.MISSING);
    }
}
