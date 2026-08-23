package com.cowlark.fluxengine.imagewriter;

import static com.google.common.truth.Truth.assertThat;
import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ImageReaderWriterType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.nio.file.Files;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class ImageWriterTest
{
    @org.junit.Rule public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    @Test
    public void createLdbsImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_LDBS).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(LdbsImageWriter.class);
    }

    @Test
    public void createD64ImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_D64).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(D64ImageWriter.class);
    }

    @Test
    public void createD88ImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_D88).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(D88ImageWriter.class);
    }

    @Test
    public void createDiskCopyImageWriter()
    {
        ImageWriterProto config = ImageWriterProto
                .newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_DISKCOPY)
                .build();

        assertThat(ImageWriter.create(config)).isInstanceOf(DiskCopyImageWriter.class);
    }

    @Test
    public void createImdImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_IMD).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(ImdImageWriter.class);
    }

    @Test
    public void createNsiImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_NSI).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(NsiImageWriter.class);
    }

    @Test
    public void createRawImageWriter()
    {
        ImageWriterProto config =
                ImageWriterProto.newBuilder().setType(ImageReaderWriterType.IMAGETYPE_RAW).build();

        assertThat(ImageWriter.create(config)).isInstanceOf(RawImageWriter.class);
    }

    @Test
    public void createImgImageWriterFromConfig()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .withImageWriter("out.dsk")
                .build();

        assertThat(ImageWriter.create(config)).isInstanceOf(ImgImageWriter.class);
    }

    @Test
    public void createBadTypeThrows()
    {
        ImageWriterProto config = ImageWriterProto
                .newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NOT_SET)
                .build();

        assertThrows(FluxEngineException.class, () -> ImageWriter.create(config));
    }

    @Test
    public void createNoWriterConfiguredThrows()
    {
        ConfigProto config = new ConfigBuilder().set("usb.serial", "test-serial").build();

        assertThrows(FluxEngineException.class, () -> ImageWriter.create(config));
    }

    @Test
    public void d64WritesSectorData() throws Exception
    {
        Image image = new Image();
        Sector sector = image.put(0, 0, 0);
        sector.data = com.cowlark.fluxengine.core.Bytes.of(1, 2, 3, 4);

        Path file = Files.createTempFile("image", ".d64");
        ImageWriterProto config = ImageWriterProto
                .newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_D64)
                .setFilename(file.toString())
                .build();

        new D64ImageWriter(config).writeImage(image);

        byte[] data = Files.readAllBytes(file);
        assertThat(data.length).isEqualTo(4);
        assertThat(data[0]).isEqualTo((byte) 1);
        assertThat(data[1]).isEqualTo((byte) 2);
        assertThat(data[2]).isEqualTo((byte) 3);
        assertThat(data[3]).isEqualTo((byte) 4);
    }

    @Test
    public void d64EmptyImageWritesNothing() throws Exception
    {
        Image image = new Image();

        Path file = Files.createTempFile("image", ".d64");
        ImageWriterProto config = ImageWriterProto
                .newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_D64)
                .setFilename(file.toString())
                .build();

        new D64ImageWriter(config).writeImage(image);

        byte[] data = Files.readAllBytes(file);
        assertThat(data).isEmpty();
    }

    @Test
    public void writeCsv() throws Exception
    {
        Image image = new Image();
        Sector sector = image.put(2, 1, 5);
        sector.status = Sector.Status.OK;
        sector.position = 1234;
        sector.clockNs = 2000.0;
        sector.headerStartTimeNs = 1.0;
        sector.headerEndTimeNs = 2.0;
        sector.dataStartTimeNs = 3.0;
        sector.dataEndTimeNs = 4.0;

        Path file = Files.createTempFile("image", ".csv");
        ImageWriter writer = new ImageWriter(ImageWriterProto.getDefaultInstance())
        {
            @Override
            public void writeImage(Image image)
            {
            }
        };

        writer.writeCsv(image, file.toString());

        String contents = Files.readString(file);
        assertThat(contents).contains("\"Physical track\",\"Physical side\"");
        assertThat(contents).contains("\"Status\"");
        assertThat(contents).contains("-1,-1,5,2,1,2000.0,1.0,2.0,3.0,4.0,1234,0,OK\n");
    }
}
