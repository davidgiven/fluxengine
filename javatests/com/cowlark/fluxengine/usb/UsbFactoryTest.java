package com.cowlark.fluxengine.usb;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class UsbFactoryTest
{
    private static class FakeUsbDevice extends UsbDevice
    {
        int closed = 0;

        @Override
        public void seek(int track)
        {
        }

        @Override
        public double getRotationalPeriod(int hardSectorCount)
        {
            return 0;
        }

        @Override
        public void testBulkWrite()
        {
        }

        @Override
        public void testBulkRead()
        {
        }

        @Override
        public Bytes read(int side, boolean synced, double readTimeNs, double hardSectorThresholdNs)
        {
            return new Bytes();
        }

        @Override
        public void write(int side, Bytes bytes, double hardSectorThresholdNs)
        {
        }

        @Override
        public void erase(int side, double hardSectorThresholdNs)
        {
        }

        @Override
        public void setDrive(int drive, boolean highDensity, int indexMode)
        {
        }

        @Override
        public VoltageMeasurements measureVoltages()
        {
            return null;
        }

        @Override
        public void close()
        {
            closed++;
        }
    }

    private static ConfigProto config()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .build();
    }

    private static void withFakeFactory(java.util.function.Function<ConfigProto, UsbDevice> factory,
                                        Runnable test)
    {
        java.util.function.Function<ConfigProto, UsbDevice> saved = UsbFactory.deviceFactory;
        UsbFactory.deviceFactory = factory;
        try
        {
            UsbFactory.reconnect(config()); /* flush any cached device */
            test.run();
        } finally
        {
            UsbFactory.deviceFactory = saved;
        }
    }

    @Test
    public void reconnectReturnsSameInstanceForSameConfig()
    {
        withFakeFactory(c -> new FakeUsbDevice(), () ->
        {
            ConfigProto config = config();

            UsbDevice first = UsbFactory.reconnect(config);
            UsbDevice second = UsbFactory.reconnect(config);

            assertThat(second).isSameInstanceAs(first);
        });
    }

    @Test
    public void reconnectCachesByConfigValue()
    {
        withFakeFactory(c -> new FakeUsbDevice(), () ->
        {
            /* The cache is keyed by ConfigProto value equality, so a distinct
             * but equal config object must hit the same cache entry. */
            ConfigProto first = config();
            ConfigProto second = config();

            UsbDevice a = UsbFactory.reconnect(first);
            UsbDevice b = UsbFactory.reconnect(second);

            assertThat(a).isNotNull();
            assertThat(b).isSameInstanceAs(a);
        });
    }

    @Test
    public void reconnectWithDifferentConfigEvictsAndClosesOldDevice()
    {
        withFakeFactory(c -> new FakeUsbDevice(), () ->
        {
            ConfigProto first = config();
            ConfigProto second = new ConfigBuilder()
                    .set("usb.serial", "test-serial")
                    .set("drive.drive", "1")
                    .build();

            UsbDevice a = UsbFactory.reconnect(first);
            FakeUsbDevice fakeA = (FakeUsbDevice) a;
            UsbDevice b = UsbFactory.reconnect(second);

            assertThat(a).isNotNull();
            assertThat(b).isNotSameInstanceAs(a);
            assertThat(fakeA.closed).isEqualTo(1);
        });
    }
}
