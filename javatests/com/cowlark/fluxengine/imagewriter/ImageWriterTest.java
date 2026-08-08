package com.cowlark.fluxengine.imagewriter;

import static com.google.common.truth.Truth.assertThat;

import static org.junit.Assert.assertThrows;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.ImageReaderWriterType;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.LogicalLocation;
import com.cowlark.fluxengine.data.Sector;
import java.nio.file.Files;
import java.nio.file.Path;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class ImageWriterTest
{
    @Test
    public void createUnportedTypeThrows()
    {
        ImageWriterProto config = ImageWriterProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_IMG)
                .build();

        assertThrows(FluxEngineException.class, () -> ImageWriter.create(config));
    }

    @Test
    public void createBadTypeThrows()
    {
        ImageWriterProto config = ImageWriterProto.newBuilder()
                .setType(ImageReaderWriterType.IMAGETYPE_NOT_SET)
                .build();

        assertThrows(FluxEngineException.class, () -> ImageWriter.create(config));
    }

    @Test
    public void createNoWriterConfiguredThrows()
    {
        ConfigProto config = new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .build();

        assertThrows(FluxEngineException.class, () -> ImageWriter.create(config));
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
        assertThat(contents).contains(
                "-1,-1,5,2,1,2000.0,1.0,2.0,3.0,4.0,1234,0,OK\n");
    }
}
