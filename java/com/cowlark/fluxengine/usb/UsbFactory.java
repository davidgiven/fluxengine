package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.config.UsbFinder;
import com.cowlark.fluxengine.config.UsbFinder.CandidateDevice;
import com.cowlark.fluxengine.core.Logger;
import lombok.SneakyThrows;
import org.slf4j.LoggerFactory;
import java.util.function.Consumer;

/**
 * USB device finder, ported from lib/usb/usbfinder.cc.
 */
public class UsbFactory implements AutoCloseable
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(UsbFactory.class);

    private final ConfigProto config;
    private UsbDevice device = null;

    public UsbFactory(ConfigProto config)
    {
        this.config = config;
    }

    @Override
    @SneakyThrows
    public void close()
    {
        if (device != null)
            device.close();
    }

    public void perform(Consumer<UsbDevice> cb)
    {
        for (; ; )
        {
            try
            {
                if (device == null)
                    device = createConnection();
                cb.accept(device);
                return;
            } catch (RetryableUsbException e)
            {
                logger.atWarn().setCause(e).log("USB threw a retryable error");
                if (device != null)
                {
                    device.close();
                    device = null;
                }
            }
        }
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
                    config,
                    config.getUsb().getApplesauce());
            case FLUXENGINE -> new FluxEngineUsbDevice(candidateDevice.device, config);
        };

        device.seek(config.getDrive().getDrive());
        return device;
    }

}
