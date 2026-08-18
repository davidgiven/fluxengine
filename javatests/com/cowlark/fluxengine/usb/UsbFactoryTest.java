package com.cowlark.fluxengine.usb;

import static com.google.common.truth.Truth.assertThat;
import static org.mockito.Mockito.verify;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.testing.TestHelpers;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;
import org.mockito.Mock;
import org.mockito.Mockito;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;
import java.util.function.Function;

@RunWith(JUnit4.class)
public class UsbFactoryTest
{
    @Rule public final TestRule loggerRule = TestHelpers.loggerRule();
    @Rule public final MockitoRule mockitoRule = MockitoJUnit.rule();

    @Mock UsbDevice mockUsbDevice;

    private static ConfigProto config()
    {
        return new ConfigBuilder().set("usb.serial", "test-serial").build();
    }

    private static void withFakeFactory(Function<ConfigProto, UsbDevice> factory, Runnable test)
    {
        Function<ConfigProto, UsbDevice> saved = UsbFactory.deviceFactory;
        UsbFactory.deviceFactory = factory;
        try
        {
            UsbFactory.getConnection(config()); /* flush any cached device */
            test.run();
        } finally
        {
            UsbFactory.deviceFactory = saved;
        }
    }

    @Test
    public void getConnectionReturnsSameInstanceForSameConfig()
    {
        withFakeFactory(c -> mockUsbDevice, () -> {
            ConfigProto config = config();

            UsbDevice first = UsbFactory.getConnection(config);
            UsbDevice second = UsbFactory.getConnection(config);

            assertThat(second).isSameInstanceAs(first);
        });
    }

    @Test
    public void getConnectionCachesByConfigValue()
    {
        withFakeFactory(c -> mockUsbDevice, () -> {
            /* The cache is keyed by ConfigProto value equality, so a distinct
             * but equal config object must hit the same cache entry. */
            ConfigProto first = config();
            ConfigProto second = config();

            UsbDevice a = UsbFactory.getConnection(first);
            UsbDevice b = UsbFactory.getConnection(second);

            assertThat(a).isNotNull();
            assertThat(b).isSameInstanceAs(a);
        });
    }

    @Test
    public void getConnectionWithDifferentConfigEvictsAndClosesOldDevice()
    {
        withFakeFactory(c -> Mockito.mock(UsbDevice.class), () -> {
            ConfigProto first = config();
            ConfigProto second = new ConfigBuilder().set("usb.serial", "test-serial")
                    .set("drive.drive", "1")
                    .build();

            UsbDevice a = UsbFactory.getConnection(first);
            UsbDevice b = UsbFactory.getConnection(second);

            assertThat(a).isNotNull();
            assertThat(b).isNotSameInstanceAs(a);
            verify(a).close();
        });
    }
}
