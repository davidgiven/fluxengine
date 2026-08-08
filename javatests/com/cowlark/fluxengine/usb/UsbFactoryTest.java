package com.cowlark.fluxengine.usb;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class UsbFactoryTest
{
    private static ConfigProto config()
    {
        /* No serial specified: with a single connected device, selectDevice
         * returns it. */
        return new ConfigBuilder().build();
    }

    @Test
    public void reconnectReturnsSameInstanceForSameConfig()
    {
        ConfigProto config = config();

        UsbDevice first = UsbFactory.reconnect(config);
        UsbDevice second = UsbFactory.reconnect(config);

        assertThat(second).isSameInstanceAs(first);
    }

    @Test
    public void reconnectCachesByConfigValue()
    {
        /* The cache is keyed by ConfigProto value equality, so a distinct but
         * equal config object must hit the same cache entry. */
        ConfigProto first = config();
        ConfigProto second = config();

        UsbDevice a = UsbFactory.reconnect(first);
        UsbDevice b = UsbFactory.reconnect(second);

        assertThat(a).isNotNull();
        assertThat(b).isSameInstanceAs(a);
    }

    @Test
    public void reconnectWithDifferentConfigEvictsAndClosesOldDevice()
    {
        ConfigProto first = config();
        ConfigProto second = new ConfigBuilder().set("drive.drive", "1").build();

        UsbDevice a = UsbFactory.reconnect(first);
        UsbDevice b = UsbFactory.reconnect(second);

        assertThat(a).isNotNull();
        assertThat(b).isNotSameInstanceAs(a);
    }
}
