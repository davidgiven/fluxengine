package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHead;
import com.cowlark.fluxengine.data.CylinderHeadSector;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.Parameterized;
import org.junit.runners.Parameterized.Parameters;
import java.io.IOException;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.List;

@RunWith(Parameterized.class)
public class TrackedBlockDeviceTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    /*
     * IBM layout: 80 cyl, 2 heads, 18 sect/track, start_sector=1.
     * Block N → track N/18, Cylinder=(N/18)/2, Head=(N/18)%2,
     * physical sector within track = (N%18)+1 (1-based).
     */
    private final int blockNumber;
    private final int seed;
    private final CylinderHead expectedTrack;
    private final int expectedPhysSector;

    public TrackedBlockDeviceTest(
            int blockNumber,
            int seed,
            CylinderHead expectedTrack,
            int expectedPhysSector)
    {
        this.blockNumber = blockNumber;
        this.seed = seed;
        this.expectedTrack = expectedTrack;
        this.expectedPhysSector = expectedPhysSector;
    }

    @Parameters(name = "block={0}, seed={1}, track={2}, phys={3}")
    public static Collection<Object[]> data()
    {
        return Arrays.asList(new Object[][]{
                /* blockNumber, seed, CylinderHead(cyl, head), physicalSector */
                {0, 10, new CylinderHead(0, 0), 1},
                {18, 20, new CylinderHead(0, 1), 1},
                {36, 30, new CylinderHead(1, 0), 1},
                {54, 40, new CylinderHead(1, 1), 1},
                {1404, 55, new CylinderHead(39, 0), 1},
                {1422, 50, new CylinderHead(39, 1), 1},
                {1440, 60, new CylinderHead(40, 0), 1},
                {9, 80, new CylinderHead(0, 0), 10},
                {27, 90, new CylinderHead(0, 1), 10},
                {1413, 70, new CylinderHead(39, 0), 10},
                {1431, 75, new CylinderHead(39, 1), 10},});
    }

    private static DiskLayout getDiskLayout()
    {
        ConfigProto configProto = new ConfigBuilder().loadConfigFile("ibm").build();
        return new DiskLayout(configProto);
    }

    private static Bytes dataBlock(int seed)
    {
        byte[] array = new byte[512];
        for (int i = 0; i < 512; i++)
            array[i] = (byte) (seed + i);
        return new Bytes(array);
    }

    @Test
    public void commitTrackMapsBlockToCorrectTrack() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.putBlock(blockNumber, dataBlock(seed));
        device.commit();

        assertThat(device.commitTrackCalls).hasSize(1);
        assertThat(device.commitTrackCalls.get(0)).isEqualTo(expectedTrack);
        assertThat(image.get(
                expectedTrack.cylinder(),
                expectedTrack.head(),
                expectedPhysSector).data.toByteArray()).isEqualTo(dataBlock(seed).toByteArray());
    }

    /* ---- Parameterised commitTrack test ---- */

    @Test
    public void populateTrackMapsBlockToCorrectTrack() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        /* Pre-populate sector at correct physical location */
        image.put(expectedTrack.cylinder(), expectedTrack.head(), expectedPhysSector).data =
                dataBlock(seed);

        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.getBlock(blockNumber);

        assertThat(device.populateTrackCalls).hasSize(1);
        assertThat(device.populateTrackCalls.get(0)).isEqualTo(expectedTrack);
        assertThat(image.get(
                expectedTrack.cylinder(),
                expectedTrack.head(),
                expectedPhysSector).data.toByteArray()).isEqualTo(dataBlock(seed).toByteArray());
    }

    /* ---- Parameterised populateTrack test ---- */

    @Test
    public void commitTrackMultipleSectorsOnSameTrack() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.putBlock(0, dataBlock(10));
        device.putBlock(1, dataBlock(20));
        device.commit();

        assertThat(device.commitTrackCalls).hasSize(2);
        assertThat(image.get(0, 0, 1).data.toByteArray()).isEqualTo(dataBlock(10).toByteArray());
        assertThat(image.get(0, 0, 2).data.toByteArray()).isEqualTo(dataBlock(20).toByteArray());
    }

    /* ---- Non-parameterised tests ---- */

    @Test
    public void commitTrackMultipleTracks() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.putBlock(0, dataBlock(10));
        device.putBlock(18, dataBlock(20));
        device.commit();

        assertThat(device.commitTrackCalls).hasSize(2);
        assertThat(image.get(0, 0, 1).data.toByteArray()).isEqualTo(dataBlock(10).toByteArray());
        assertThat(image.get(0, 1, 1).data.toByteArray()).isEqualTo(dataBlock(20).toByteArray());
    }

    @Test
    public void populateTrackMultipleTracks() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        image.put(0, 0, 1).data = dataBlock(10);
        image.put(0, 1, 1).data = dataBlock(20);

        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.getBlock(0);  /* (C0, H0) */
        device.getBlock(18); /* (C0, H1) */

        assertThat(device.populateTrackCalls).hasSize(2);
    }

    @Test
    public void populateTrackDeduplicatesAcrossBlocks() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        image.put(0, 0, 1).data = dataBlock(10);
        image.put(0, 0, 2).data = dataBlock(20);

        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.getBlock(0);
        device.getBlock(1);

        assertThat(device.populateTrackCalls).hasSize(1);
        assertThat(device.populateTrackCalls.get(0)).isEqualTo(new CylinderHead(0, 0));
        assertThat(device.getBlock(0).toByteArray()).isEqualTo(dataBlock(10).toByteArray());
        assertThat(device.getBlock(1).toByteArray()).isEqualTo(dataBlock(20).toByteArray());
    }

    @Test
    public void writeCommitReadRoundTrip() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        device.putBlock(0, dataBlock(10));
        device.putBlock(18, dataBlock(20));
        device.commit();

        RecordingTrackedBlockDevice device2 = new RecordingTrackedBlockDevice(diskLayout, image);

        assertThat(device2.getBlock(0).toByteArray()).isEqualTo(dataBlock(10).toByteArray());
        assertThat(device2.getBlock(18).toByteArray()).isEqualTo(dataBlock(20).toByteArray());
        assertThat(device2.populateTrackCalls).hasSize(2);
    }

    @Test
    public void readBlockFromImageBeforeAnyWrite() throws IOException
    {
        DiskLayout diskLayout = getDiskLayout();
        Image image = new Image();
        for (int i = 0; i < 72; i++)
        {
            CylinderHeadSector loc = diskLayout.logicalSectorLocationsInFilesystemOrder.get(i);
            image.put(loc).data = dataBlock(i * 50);
        }

        RecordingTrackedBlockDevice device = new RecordingTrackedBlockDevice(diskLayout, image);

        assertThat(device.getBlock(0).toByteArray()).isEqualTo(dataBlock(0).toByteArray());
        assertThat(device.getBlock(18).toByteArray()).isEqualTo(dataBlock(900).toByteArray());
        assertThat(device.getBlock(36).toByteArray()).isEqualTo(dataBlock(1800).toByteArray());

        assertThat(device.populateTrackCalls).hasSize(3);
    }

    /* A RecordingTrackedBlockDevice captures commitTrack / populateTrack calls. */
    private static class RecordingTrackedBlockDevice extends TrackedBlockDevice
    {
        final Image image;
        final List<CylinderHead> commitTrackCalls = new ArrayList<>();
        final List<CylinderHead> populateTrackCalls = new ArrayList<>();

        RecordingTrackedBlockDevice(DiskLayout diskLayout, Image image)
        {
            super(diskLayout);
            this.image = image;
        }

        @Override
        protected void commitTrack(Image source, CylinderHead lch)
        {
            commitTrackCalls.add(lch);
            copySectors(source, image, lch);
        }

        @Override
        protected void populateTrack(Image destination, CylinderHead lch)
        {
            populateTrackCalls.add(lch);
            copySectors(image, destination, lch);
        }
    }
}
