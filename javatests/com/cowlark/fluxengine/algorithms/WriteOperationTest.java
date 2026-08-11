package com.cowlark.fluxengine.algorithms;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.data.DiskLayout;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class WriteOperationTest
{
    @org.junit.Rule
    public final org.junit.rules.TestRule loggerRule =
            com.cowlark.fluxengine.testing.TestHelpers.loggerRule();

    private static ConfigProto makeConfig()
    {
        return new ConfigBuilder().set("usb.serial", "test-serial")
                .set("drive.rotational_period_ms", "200")
                .set("layout.tracks", "1")
                .set("layout.sides", "1")
                .set("layout.layoutdata[0].sector_size", "256")
                .set("layout.layoutdata[0].physical.start_sector", "0")
                .set("layout.layoutdata[0].physical.count", "8")
                .build();
    }

    @Test
    public void getConfigReturnsConfiguredConfig()
    {
        ConfigProto config = makeConfig();
        WriteOperation operation = new WriteOperation(config);

        assertThat(operation.getConfig()).isSameInstanceAs(config);
    }

    @Test
    public void getDiskLayoutBuildsFromConfig()
    {
        WriteOperation operation = new WriteOperation(makeConfig());

        DiskLayout diskLayout = operation.getDiskLayout();

        assertThat(diskLayout).isNotNull();
        assertThat(diskLayout.logicalLocations).isNotEmpty();
        assertThat(diskLayout.layoutByLogicalLocation.size()).isEqualTo(1);
    }

    @Test
    public void getDiskLayoutIsMemoized()
    {
        WriteOperation operation = new WriteOperation(makeConfig());

        assertThat(operation.getDiskLayout()).isSameInstanceAs(operation.getDiskLayout());
    }

    @Test
    public void closeDoesNotThrowWhenNothingCreated()
    {
        WriteOperation operation = new WriteOperation(makeConfig());

        try
        {
            operation.close();
        } catch (Exception e)
        {
            throw new AssertionError("close should not throw", e);
        }
    }
}
