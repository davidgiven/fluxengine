package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.SupplierOfAutocloseable;
import lombok.SneakyThrows;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public class UsbFactory implements AutoCloseable
{
    private final ConfigProto config;
    private final SupplierOfAutocloseable<UsbDevice> deviceSupplier =
            new SupplierOfAutocloseable<>(this::createConnection);

    public UsbFactory(ConfigProto config)
    {
        this.config = config;
    }

    @Override
    @SneakyThrows
    public void close()
    {
        deviceSupplier.close();
    }

    public UsbDevice getConnection()
    {
        return deviceSupplier.get();
    }

    public UsbDevice createConnection()
    {
        CandidateDevice candidateDevice = UsbFinder.selectDevice(config);
        Logger.logf("connecting to %s serial %s",
                candidateDevice.type.getDeviceName(),
                candidateDevice.serial);
        UsbDevice device = switch (candidateDevice.type)
        {
            case GREASEWEAZLE -> new GreaseweazleUsbDevice(candidateDevice.serialPort, config);
            case APPLESAUCE -> new ApplesauceUsbDevice(candidateDevice.serialPort,
                    config, config.getUsb().getApplesauce());
            case FLUXENGINE -> new FluxEngineUsbDevice(candidateDevice.device, config);
            default -> throw new FluxEngineException("unsupported hardware device");

        };

        device.seek(config.getDrive().getDrive());
        return device;
    }

}
