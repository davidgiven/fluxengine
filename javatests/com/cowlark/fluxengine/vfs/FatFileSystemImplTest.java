package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.FileSystemImpl.FileType.IS_FILE;
import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.testing.TestHelpers;
import com.cowlark.fluxengine.vfs.FileSystemImpl.Dirent;
import com.google.common.collect.ImmutableMap;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;
import java.nio.file.Path;

@RunWith(JUnit4.class)
public class FatFileSystemImplTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;
    private InMemoryBlockDevice blockDevice;
    private FatFileSystemImpl impl;

    @Before
    public void setup()
    {
        configProto =
                new ConfigBuilder().loadConfigFile("ibm").withImageWriter("/tmp/out.img").build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new FatFileSystemImpl(configProto.getFilesystem().getFatfs(), blockDevice);
    }

    @Test
    public void createFilesystem() throws IOException
    {
        impl.create(true, "LABEL");
        blockDevice.commit();
        assertThat(image.get(0, 0, 1).data.reader().seek(3).readString(8)).isEqualTo("fluxengn");
        assertThat(image.get(0, 0, 2).data
                .reader()
                .read(3)
                .toByteArray()).isEqualTo(new byte[]{(byte) 0xf8, (byte) 0xff, (byte) 0xff});
        assertThat(image.get(0, 1, 8).data.reader().readString(5)).isEqualTo("LABEL");
    }

    @Test
    public void putFile() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("data"), new Bytes("Hello, world!"));
        blockDevice.commit();
        assertThat(image.get(1, 1, 4).data.reader().readString(13)).isEqualTo("Hello, world!");
    }

    @Test
    public void listFiles() throws IOException
    {
        impl.create(true, "LABEL");
        impl.putFile(Path.of("data"), new Bytes("Hello, world!"));
        ImmutableMap<String, Dirent> files = impl.list(Path.of("/"));
        assertThat(files).hasSize(1);
        assertThat(files.get("data")).isEqualTo(Dirent
                .builder()
                .setPath(Path.of("/data"))
                .setFilename("data")
                .setLength(13)
                .setFileType(IS_FILE)
                .setMode("")
                .setAttributes(ImmutableMap
                        .<String, String>builder()
                        .put(Attributes.FILENAME, "data")
                        .put(Attributes.LENGTH, "13")
                        .put(Attributes.FILE_TYPE, "file")
                        .build())
                .build());
    }

    /* Do not use --- for debugging the test only */
    private void writeImage()
    {
        ImageWriter.create(configProto).writeImage(image);
    }
}
