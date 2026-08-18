package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.core.FluxEngineException;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.util.List;

@RunWith(JUnit4.class)
public class LocationsTest
{
    @Test
    public void parseSingle()
    {
        assertThat(Locations.parseCylinderHeadsString("c0h0")).containsExactly(new CylinderHead(0,
                0));
    }

    @Test
    public void parseRangeAndStep()
    {
        assertThat(Locations.parseCylinderHeadsString("c0-2h0-2x2")).containsExactly(new CylinderHead(
                        0,
                        0),
                new CylinderHead(0, 2),
                new CylinderHead(1, 0),
                new CylinderHead(1, 2),
                new CylinderHead(2, 0),
                new CylinderHead(2, 2));
    }

    @Test
    public void parseMultipleGroups()
    {
        assertThat(Locations.parseCylinderHeadsString("c1h1 c0h0")).containsExactly(new CylinderHead(
                0,
                0), new CylinderHead(1, 1));
    }

    @Test
    public void convertRoundTrip()
    {
        List<CylinderHead> chs = List.of(new CylinderHead(0, 0), new CylinderHead(1, 2));

        assertThat(Locations.convertCylinderHeadsToString(chs)).isEqualTo("c0h0 c1h2");
    }

    @Test
    public void parseMalformedThrows()
    {
        assertThrows(FluxEngineException.class, () -> Locations.parseCylinderHeadsString("c0"));
        assertThrows(FluxEngineException.class,
                () -> Locations.parseCylinderHeadsString("garbage"));
        assertThrows(FluxEngineException.class, () -> Locations.parseCylinderHeadsString("c0h2x0"));
    }
}
