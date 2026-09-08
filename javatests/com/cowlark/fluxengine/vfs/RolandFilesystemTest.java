package com.cowlark.fluxengine.vfs;

import static com.cowlark.fluxengine.vfs.RolandFilesystem.BLOCK_SIZE;

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
public class RolandFilesystemTest extends GenericFilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto configProto;
    private DiskLayout diskLayout;
    private Image image;

    @Before
    public void setup()
    {
        configProto = new ConfigBuilder()
                .loadConfigFile("rolandd20")
                .withImageWriter("/tmp/out.img")
                .build();
        diskLayout = new DiskLayout(configProto);
        image = new Image();
        createTestFilesystem();
    }

    @Override
    protected Bytes getTestFileData(String contents)
    {
        return super.getTestFileData(contents).slice(0, BLOCK_SIZE);
    }

    @Override
    public void createTestFilesystem()
    {
        blockDevice = new InMemoryBlockDevice(diskLayout, image);
        impl = new RolandFilesystem(configProto.getFilesystem().getRoland(), blockDevice);
    }
}
