package com.cowlark.fluxengine.imagereader;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ImageReaderWriterType;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ImageReaderTest
{
    @org.junit.Rule
    public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    @Test
    public void createD64ImageReader()
    {
        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_D64)
                .build();

        assertThat(ImageReader.create(config)).isInstanceOf(D64ImageReader.class);
    }

    @Test
    public void createImgImageReader()
    {
        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_IMG)
                .build();

        assertThat(ImageReader.create(ConfigProto.getDefaultInstance(), config))
                .isInstanceOf(ImgImageReader.class);
    }

    @Test
    public void createNsiImageReader()
    {
        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NSI)
                .build();

        assertThat(ImageReader.create(config)).isInstanceOf(NsiImageReader.class);
    }

    @Test
    public void createTd0ImageReader()
    {
        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_TD0)
                .build();

        assertThat(ImageReader.create(config)).isInstanceOf(Td0ImageReader.class);
    }

    @Test
    public void createBadTypeThrows()
    {
        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NOT_SET)
                .build();

        assertThrows(FluxEngineException.class, () -> ImageReader.create(config));
    }

    @Test
    public void createNoReaderConfiguredThrows()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .build();

        assertThrows(FluxEngineException.class, () -> ImageReader.create(config));
    }

    @Test
    public void d64ReadsSectorData() throws Exception
    {
        /* 40 tracks; the first track has 21 sectors of 256 bytes. Write a
         * single byte of payload at the start of sector 0. */
        Path file = Files.createTempFile("image", ".d64");
        byte[] data = new byte[256 * 21];
        data[0] = 0x42;
        Files.write(file, data);

        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_D64)
                .setFilename(file.toString())
                .build();

        Image image = new D64ImageReader(config).readImage();

        Sector sector = image.get(0, 0, 0);
        assertThat(sector).isNotNull();
        assertThat(sector.status).isEqualTo(Sector.Status.OK);
        assertThat(sector.data.getByte(0) & 0xff).isEqualTo(0x42);
        assertThat(sector.data.size()).isEqualTo(256);
    }

    @Test
    public void d64ShortFileMarksMissing() throws Exception
    {
        Path file = Files.createTempFile("image", ".d64");
        Files.write(file, new byte[10]);

        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_D64)
                .setFilename(file.toString())
                .build();

        Image image = new D64ImageReader(config).readImage();

        /* Track 0, sector 0 is present (10 bytes available); track 39, sector
         * 0 has no data. */
        assertThat(image.get(0, 0, 0).status).isEqualTo(Sector.Status.OK);
        assertThat(image.get(39, 0, 0).status).isEqualTo(Sector.Status.DATA_MISSING);
    }

    @Test
    public void nsiReadsSectorData() throws Exception
    {
        /* 35 tracks x 2 heads x 10 sectors x 512 bytes. */
        Path file = Files.createTempFile("image", ".nsi");
        byte[] data = new byte[358400];
        data[0] = 0x43;
        Files.write(file, data);

        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NSI)
                .setFilename(file.toString())
                .build();

        Image image = new NsiImageReader(config).readImage();

        Sector sector = image.get(0, 0, 0);
        assertThat(sector).isNotNull();
        assertThat(sector.data.getByte(0) & 0xff).isEqualTo(0x43);
        assertThat(image.get(34, 1, 0)).isNotNull();
    }

    @Test
    public void nsiUnknownSizeThrows() throws Exception
    {
        Path file = Files.createTempFile("image", ".nsi");
        Files.write(file, new byte[12345]);

        ImageReaderProto config = ImageReaderProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NSI)
                .setFilename(file.toString())
                .build();

        assertThrows(FluxEngineException.class, () -> new NsiImageReader(config).readImage());
    }
}
