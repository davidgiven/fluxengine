package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class FilesystemTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();

    private ConfigProto config;

    @Before
    public void setup()
    {
        config = new ConfigBuilder().loadConfigFile("ibm").withImageWriter("/tmp/out.img").build();
    }

    @Test
    public void doWithFilesystem_startsAndShutsDownThread()
    {
        Filesystem.doWithFilesystem(
                config, fs -> {
                });
    }

    @Test
    public void doWithFilesystem_canBeInvokedTwice()
    {
        Filesystem.doWithFilesystem(
                config, fs -> {
                });
        Filesystem.doWithFilesystem(
                config, fs -> {
                });
    }
}
