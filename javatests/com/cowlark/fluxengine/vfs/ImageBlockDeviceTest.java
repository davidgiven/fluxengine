package com.cowlark.fluxengine.vfs;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class ImageBlockDeviceTest
{
    public static final Bytes DATA1 = Bytes.of(1, 2, 3, 4).slice(0, 512);
    public static final Bytes DATA2 = Bytes.of(4, 3, 2, 1).slice(0, 512);

    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private Path tempFile;
    private ConfigProto configProto;

    @Before
    public void setup() throws Exception
    {
        tempFile = Files.createTempFile("image", ".img");
        configProto = new ConfigBuilder()
                .loadConfigFile("ibm")
                .withImageWriter(tempFile.toString())
                .withImageReader(tempFile.toString())
                .build();

        // Write an empty image so the file exists with correct size
        Image empty = new Image();
        // Create an image with all sectors zeroed for consistent reads
        // ImgImageWriter will pad missing sectors, so writing empty image
        // creates a zero-filled file of correct size
        ImageWriter.create(configProto).writeImage(empty);
    }

    private FilesystemOperation createOperation()
    {
        FilesystemOperation fso = new FilesystemOperation(fs -> {
        });
        fso.setConfig(configProto);
        fso.init();
        return fso;
    }

    private void writeImageWithBlock0(Bytes data) throws Exception
    {
        Image image = new Image();
        // Use logical location for block 0 (track 0, head 0, sector 1 for IBM)
        image.put(0, 0, 1).data = data;
        ImageWriter.create(configProto).writeImage(image);
    }

    @Test
    public void testBlockCountAndBlockSize() throws Exception
    {
        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        assertThat(device.getBlockCount()).isEqualTo(2880);
        assertThat(device.getBlockSize()).isEqualTo(512);
    }

    @Test
    public void readBlocks() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        Bytes block0 = device.getBlock(0);
        assertThat(block0).isEqualTo(DATA1);
    }

    @Test
    public void writeThenReadBlocks() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
    }

    @Test
    public void writeThenCommit() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        // oldImage still has old data before commit
        assertThat(device.oldImage.get(0, 0, 1).data).isEqualTo(DATA1);
        device.commit();
        assertThat(device.oldImage.get(0, 0, 1).data).isEqualTo(DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(device.needsCommit()).isFalse();
    }

    @Test
    public void writeThenRevert() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        device.putBlock(0, DATA2);
        assertThat(device.getBlock(0)).isEqualTo(DATA2);
        assertThat(device.oldImage.get(0, 0, 1).data).isEqualTo(DATA1);
        device.revert();
        assertThat(device.getBlock(0)).isEqualTo(DATA1);
        assertThat(device.oldImage.get(0, 0, 1).data).isEqualTo(DATA1);
        assertThat(device.needsCommit()).isFalse();
    }

    @Test
    public void needsCommitTracksChanges() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);

        assertThat(device.needsCommit()).isFalse();
        device.putBlock(0, DATA2);
        assertThat(device.needsCommit()).isTrue();
        device.commit();
        assertThat(device.needsCommit()).isFalse();
        device.putBlock(0, DATA1);
        assertThat(device.needsCommit()).isTrue();
        device.revert();
        assertThat(device.needsCommit()).isFalse();
    }

    @Test
    public void commitPersistsToFile() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);
        device.putBlock(0, DATA2);
        device.commit();

        // New device reading same file should see committed data
        FilesystemOperation fso2 = createOperation();
        ImageBlockDevice device2 = new ImageBlockDevice(fso2);
        assertThat(device2.getBlock(0)).isEqualTo(DATA2);
    }

    @Test
    public void revertDoesNotPersistToFile() throws Exception
    {
        writeImageWithBlock0(DATA1);

        FilesystemOperation fso = createOperation();
        ImageBlockDevice device = new ImageBlockDevice(fso);
        device.putBlock(0, DATA2);
        device.revert();

        FilesystemOperation fso2 = createOperation();
        ImageBlockDevice device2 = new ImageBlockDevice(fso2);
        assertThat(device2.getBlock(0)).isEqualTo(DATA1);
    }
}
