package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.CylinderHeadSector;
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

    private Bytes sequentialBlock(int seed)
    {
        byte[] array = new byte[512];
        for (int i = 0; i < 512; i++)
            array[i] = (byte) (seed + i);
        return new Bytes(array);
    }

    private Image createImageWithSequentialBlocks(int count)
    {
        Image image = new Image();
        for (int i = 0; i < count; i++)
        {
            CylinderHeadSector loc = diskLayout.logicalSectorLocationsInFilesystemOrder.get(i);
            com.cowlark.fluxengine.data.Sector sector = image.put(loc);
            sector.data = sequentialBlock(i * 50);
        }
        return image;
    }

    @Test
    public void getBytesAlignedSingleBlock() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        Bytes block0 = sequentialBlock(0);
        Bytes result = device.getBytes(0, 512);

        assertThat(result.toByteArray()).isEqualTo(block0.toByteArray());
    }

    @Test
    public void getBytesUnalignedSingleBlock() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        Bytes result = device.getBytes(100, 50);
        Bytes expected = sequentialBlock(0).slice(100, 50);

        assertThat(result.toByteArray()).isEqualTo(expected.toByteArray());
    }

    @Test
    public void getBytesSpanningTwoBlocksUnaligned() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        // Offset 500 spans tail of block 0 and head of block 1
        Bytes result = device.getBytes(500, 24);
        Bytes block0 = sequentialBlock(0);
        Bytes block1 = sequentialBlock(50);
        Bytes expected = block0.slice(500, 12).concat(block1.slice(0, 12));

        assertThat(result.toByteArray()).isEqualTo(expected.toByteArray());
        assertThat(result.size()).isEqualTo(24);
    }

    @Test
    public void getBytesSpanningMultipleBlocks() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        // Offset 500, length 600 spans 12 bytes of block 0, all of block 1, and 76 bytes of block 2
        Bytes result = device.getBytes(500, 600);
        Bytes block0 = sequentialBlock(0);
        Bytes block1 = sequentialBlock(50);
        Bytes block2 = sequentialBlock(100);
        Bytes expected = block0.slice(500, 12).concat(block1).concat(block2.slice(0, 76));

        assertThat(result.toByteArray()).isEqualTo(expected.toByteArray());
        assertThat(result.size()).isEqualTo(600);
    }

    @Test
    public void putBytesAlignedSingleBlock() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        Bytes newData = sequentialBlock(200);
        device.putBytes(0, newData);

        Bytes result = device.getBlock(0);
        assertThat(result.toByteArray()).isEqualTo(newData.toByteArray());
        // Adjacent block unchanged
        assertThat(device.getBlock(1).toByteArray()).isEqualTo(sequentialBlock(50).toByteArray());
    }

    @Test
    public void putBytesUnalignedSingleBlock() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        // Write 20 bytes at offset 10 within block 0
        byte[] patchArray = new byte[20];
        for (int i = 0; i < 20; i++)
            patchArray[i] = (byte) (0xF0 + i);
        Bytes patch = new Bytes(patchArray);

        device.putBytes(10, patch);

        Bytes block0 = device.getBlock(0);
        byte[] expectedArray = sequentialBlock(0).toByteArray();
        System.arraycopy(patchArray, 0, expectedArray, 10, 20);
        Bytes expected = new Bytes(expectedArray);

        assertThat(block0.toByteArray()).isEqualTo(expected.toByteArray());
        // Next block unchanged
        assertThat(device.getBlock(1).toByteArray()).isEqualTo(sequentialBlock(50).toByteArray());
    }

    @Test
    public void putBytesSpanningBlocksUnaligned() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        // Write 24 bytes spanning tail of block 0 (12 bytes) and head of block 1 (12 bytes)
        byte[] patchArray = new byte[24];
        for (int i = 0; i < 24; i++)
            patchArray[i] = (byte) (0xA0 + i);
        Bytes patch = new Bytes(patchArray);

        device.putBytes(500, patch);

        Bytes block0 = device.getBlock(0);
        Bytes block1 = device.getBlock(1);
        byte[] expected0Array = sequentialBlock(0).toByteArray();
        byte[] expected1Array = sequentialBlock(50).toByteArray();
        System.arraycopy(patchArray, 0, expected0Array, 500, 12);
        System.arraycopy(patchArray, 12, expected1Array, 0, 12);
        Bytes expected0 = new Bytes(expected0Array);
        Bytes expected1 = new Bytes(expected1Array);

        assertThat(block0.toByteArray()).isEqualTo(expected0.toByteArray());
        assertThat(block1.toByteArray()).isEqualTo(expected1.toByteArray());
    }

    @Test
    public void putBytesSpanningMultipleBlocks() throws IOException
    {
        Image image = createImageWithSequentialBlocks(4);
        InMemoryBlockDevice device = new InMemoryBlockDevice(diskLayout, image);

        // Write 600 bytes spanning block 0 tail, all of block 1, and part of block 2
        byte[] patchArray = new byte[600];
        for (int i = 0; i < 600; i++)
            patchArray[i] = (byte) (0xC0 + (i % 16));
        Bytes patch = new Bytes(patchArray);

        device.putBytes(500, patch);

        Bytes block0 = device.getBlock(0);
        Bytes block1 = device.getBlock(1);
        Bytes block2 = device.getBlock(2);
        byte[] expected0Array = sequentialBlock(0).toByteArray();
        byte[] expected1Array = sequentialBlock(50).toByteArray();
        byte[] expected2Array = sequentialBlock(100).toByteArray();
        System.arraycopy(patchArray, 0, expected0Array, 500, 12);
        System.arraycopy(patchArray, 12, expected1Array, 0, 512);
        System.arraycopy(patchArray, 524, expected2Array, 0, 76);

        assertThat(block0.toByteArray()).isEqualTo(new Bytes(expected0Array).toByteArray());
        assertThat(block1.toByteArray()).isEqualTo(new Bytes(expected1Array).toByteArray());
        assertThat(block2.toByteArray()).isEqualTo(new Bytes(expected2Array).toByteArray());

        // Verify round-trip via getBytes
        Bytes roundTrip = device.getBytes(500, 600);
        assertThat(roundTrip.toByteArray()).isEqualTo(patchArray);
    }
}
