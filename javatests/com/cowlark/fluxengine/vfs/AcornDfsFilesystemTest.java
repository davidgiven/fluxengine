package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.data.Image;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class AcornDfsFilesystemTest extends GenericFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto = new ConfigBuilder()
                .loadConfigFile("acorndfs")
                .withImageWriter("/tmp/out.img")
                .build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new AcornDfsFilesystem(configProto.getFilesystem().getAcorndfs(), blockDevice);
    }
}
