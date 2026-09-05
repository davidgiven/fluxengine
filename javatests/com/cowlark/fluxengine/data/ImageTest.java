package com.cowlark.fluxengine.data;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ImageTest
{
    private static Sector makeSector(int cylinder, int head, int sector, int size)
    {
        Sector s = new Sector(new CylinderHeadSector(cylinder, head, sector));
        s.data = new Bytes(size);
        return s;
    }

    @Test
    public void emptyImageHasNoSectors()
    {
        Image image = new Image();

        assertThat(image.empty()).isTrue();
        assertThat(image.iterator().hasNext()).isFalse();
    }

    @Test
    public void putAndGetSectors()
    {
        Image image = new Image();

        Sector sector = image.put(0, 0, 3);
        assertThat(image.contains(0, 0, 3)).isTrue();
        assertThat(image.contains(new CylinderHeadSector(0, 0, 3))).isTrue();
        assertThat(image.get(0, 0, 3)).isSameInstanceAs(sector);
        assertThat(image.get(new CylinderHeadSector(0, 0, 3))).isSameInstanceAs(sector);

        image.erase(0, 0, 3);
        assertThat(image.contains(0, 0, 3)).isFalse();
        assertThat(image.get(0, 0, 3)).isNull();
    }

    @Test
    public void calculatesGeometry()
    {
        Image image = new Image();
        image.put(0, 0, 1).data = new Bytes(128);
        image.put(2, 1, 5).data = new Bytes(256);
        image.put(2, 1, 8).data = new Bytes(512);

        image.calculateSize();

        Geometry geometry = image.getGeometry();
        assertThat(geometry.numCylinders).isEqualTo(3);
        assertThat(geometry.numHeads).isEqualTo(2);
        assertThat(geometry.firstSector).isEqualTo(1);
        assertThat(geometry.numSectors).isEqualTo(8);
        assertThat(geometry.sectorSize).isEqualTo(512);
        assertThat(geometry.totalBytes).isEqualTo(896);
    }

    @Test
    public void constructorCalculatesGeometry()
    {
        java.util.List<Sector> sectors =
                java.util.List.of(makeSector(0, 0, 0, 256), makeSector(1, 1, 3, 256));

        Image image = new Image(sectors);

        assertThat(image.getGeometry().numCylinders).isEqualTo(2);
        assertThat(image.getGeometry().numHeads).isEqualTo(2);
        assertThat(image.getGeometry().firstSector).isEqualTo(0);
        assertThat(image.getGeometry().numSectors).isEqualTo(4);
    }

    @Test
    public void addMissingSectorsPopulatesMissing()
    {
        Image image = new Image();
        image.put(0, 0, 0);

        /* A disk with sectors 0 and 1; sector 1 is missing. */
        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        image.addMissingSectors(layout, false);

        assertThat(image.contains(0, 0, 0)).isTrue();
        assertThat(image.contains(0, 0, 1)).isTrue();
        assertThat(image.get(0, 0, 1).status).isEqualTo(Sector.Status.MISSING);
    }

    @Test
    public void populateSectorPhysicalLocations()
    {
        Image image = new Image();
        image.put(0, 0, 0);
        image.put(0, 0, 1);

        DiskLayout layout = new DiskLayout(1, 1, 2, 256);
        image.populateSectorPhysicalLocationsFromLogicalLocations(layout);

        for (Sector sector : image)
        {
            assertThat(sector.physicalLocation).isNotNull();
            assertThat(sector.physicalLocation.cylinder()).isEqualTo(sector.logicalLocation.cylinder());
            assertThat(sector.physicalLocation.head()).isEqualTo(sector.logicalLocation.head());
        }
    }
}
