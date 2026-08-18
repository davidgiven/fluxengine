package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class SectorTest
{
    @Test
    public void defaultsAreEmptySector()
    {
        Sector sector = new Sector(new LogicalLocation(0, 0, 0));

        assertThat(sector.location).isEqualTo(new LogicalLocation(0, 0, 0));
        assertThat(sector.status).isEqualTo(Sector.Status.INTERNAL_ERROR);
        assertThat(sector.position).isEqualTo(0);
        assertThat(sector.clockNs).isEqualTo(0.0);
        assertThat(sector.headerStartTimeNs).isEqualTo(0.0);
        assertThat(sector.headerEndTimeNs).isEqualTo(0.0);
        assertThat(sector.dataStartTimeNs).isEqualTo(0.0);
        assertThat(sector.dataEndTimeNs).isEqualTo(0.0);
        assertThat(sector.physicalLocation).isNull();
        assertThat(sector.data.isEmpty()).isTrue();
        assertThat(sector.records).isEmpty();
    }

    @Test
    public void holdsLogicalLocation()
    {
        LogicalLocation location = new LogicalLocation(1, 2, 3);
        Sector sector = new Sector(location);

        assertThat(sector.location).isSameInstanceAs(location);
        assertThat(sector.location.trackLocation()).isEqualTo(new CylinderHead(1, 2));
    }

    @Test
    public void statusStringRoundTrips()
    {
        for (Sector.Status status : Sector.Status.values())
        {
            assertThat(Sector.stringToStatus(Sector.statusToString(status))).isEqualTo(status);
        }
    }

    @Test
    public void statusToStringIsReadable()
    {
        assertThat(Sector.statusToString(Sector.Status.OK)).isEqualTo("OK");
        assertThat(Sector.statusToString(Sector.Status.MISSING)).isEqualTo("sector not found");
        assertThat(Sector.statusToString(Sector.Status.DATA_MISSING)).isEqualTo(
                "present but no data found");
    }

    @Test
    public void stringToStatusAcceptsChars()
    {
        assertThat(Sector.stringToStatus("OK")).isEqualTo(Sector.Status.OK);
        assertThat(Sector.stringToStatus("MISSING")).isEqualTo(Sector.Status.MISSING);
        assertThat(Sector.stringToStatus("bad checksum")).isEqualTo(Sector.Status.BAD_CHECKSUM);
        assertThat(Sector.stringToStatus("garbage")).isEqualTo(Sector.Status.INTERNAL_ERROR);
    }
}
