package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;

@RunWith(JUnit4.class)
public class InMemoryBlockDeviceTest
{
    public static final Bytes DATA1 = Bytes.of(1, 2, 3, 4).slice(0, 512);
    public static final Bytes DATA2 = Bytes.of(4, 3, 2, 1).slice(0, 512);

    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;

    @Before
    public void setup()
    {
        configProto = new ConfigBuilder().loadConfigFile("ibm").build();
        diskLayout = new DiskLayout(configProto);
    }

    @Test
    public void testBlockCountAndBlockSize()
    {
        Image image = new Image();
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        assertThat(device.getBlockCount()).isEqualTo(2880);
        assertThat(device.getBlockSize()).isEqualTo(512);
    }

    @Test
    public void readBlocks() throws IOException
    {
        Image image = new Image();
        image.put(0, 0, 1).data = DATA1;
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        Bytes block0 = device.getBlock(0);
        assertThat(block0).isEqualTo(DATA1);
    }

    @Test
    public void writeThenReadBlocks() throws IOException
    {
        Image image = new Image();
        image.put(0, 0, 1).data = DATA1;
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
    }

    @Test
    public void writeThenCommit() throws IOException
    {
        Image image = new Image();
        image.put(0, 0, 1).data = DATA1;
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(image.get(0, 0, 1).data).isEqualTo(DATA1);
        device.commit();
        assertThat(image.get(0, 0, 1).data).isEqualTo(DATA2);
    }

    @Test
    public void writeThenRevert() throws IOException
    {
        Image image = new Image();
        image.put(0, 0, 1).data = DATA1;
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(image.get(0, 0, 1).data).isEqualTo(DATA1);
        device.revert();
        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        assertThat(image.get(0, 0, 1).data).isEqualTo(DATA1);
    }
}
