package com.cowlark.fluxengine.fluxsource;

import static com.google.common.truth.Truth.assertThat;

import com.cowlark.fluxengine.config.ConfigBuilder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.Bytes;
import com.cowlark.fluxengine.data.Fluxmap;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.VoltageMeasurements;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.JUnit4;

@RunWith(JUnit4.class)
public class HardwareFluxSourceTest
{
    private static class FakeUsbDevice extends UsbDevice
    {
        int seekedTo = -1;
        int recalibrated = 0;
        Integer readSide;
        Boolean readSynced;
        Double readTimeNs;
        Double readThresholdNs;
        Bytes readResult = new Bytes();

        @Override
        public void seek(int track)
        {
            seekedTo = track;
        }

        @Override
        public void recalibrate()
        {
            recalibrated++;
            seek(0);
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
            readSide = side;
            readSynced = synced;
            this.readTimeNs = readTimeNs;
            readThresholdNs = hardSectorThresholdNs;
            return readResult;
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
        }
    }

    private static ConfigProto config()
    {
        return new ConfigBuilder()
                .set("usb.serial", "test-serial")
                .set("drive.sync_with_index", "true")
                .set("drive.revolutions", "3")
                .set("drive.rotational_period_ms", "200")
                .set("drive.hard_sector_threshold_ns", "1000")
                .build();
    }

    @Test
    public void isHardware()
    {
        HardwareFluxSource source = new HardwareFluxSource(config(), new FakeUsbDevice());

        assertThat(source.isHardware()).isTrue();
    }

    @Test
    public void seekDelegatesToDevice()
    {
        FakeUsbDevice device = new FakeUsbDevice();
        HardwareFluxSource source = new HardwareFluxSource(config(), device);

        source.seek(42);

        assertThat(device.seekedTo).isEqualTo(42);
    }

    @Test
    public void recalibrateDelegatesToDevice()
    {
        FakeUsbDevice device = new FakeUsbDevice();
        HardwareFluxSource source = new HardwareFluxSource(config(), device);

        source.recalibrate();

        assertThat(device.recalibrated).isEqualTo(1);
    }

    @Test
    public void readFluxReadsAndWrapsFluxmap()
    {
        FakeUsbDevice device = new FakeUsbDevice();
        device.readResult = Bytes.of(0x01, 0x02, 0x03, 0x04);
        HardwareFluxSource source = new HardwareFluxSource(config(), device);

        FluxSourceIterator iterator = source.readFlux(17, 1);

        assertThat(iterator.hasNext()).isTrue();
        Fluxmap fluxmap = iterator.next();

        assertThat(device.seekedTo).isEqualTo(17);
        assertThat(device.readSide).isEqualTo(1);
        assertThat(device.readSynced).isTrue();
        assertThat(device.readTimeNs).isEqualTo(3 * 200 * 1e6);
        assertThat(device.readThresholdNs).isEqualTo(1000);
        assertThat(fluxmap.rawBytes()).isEqualTo(device.readResult);
        assertThat(iterator.hasNext()).isTrue();
    }
}
