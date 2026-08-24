package com.cowlark.fluxengine.vfs;


import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.data.Sector;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import java.io.IOException;

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

}
