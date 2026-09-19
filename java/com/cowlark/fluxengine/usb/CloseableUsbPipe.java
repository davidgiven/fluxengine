package com.cowlark.fluxengine.usb;

import com.cowlark.fluxengine.core.FluxEngineException;
import org.slf4j.LoggerFactory;
import javax.usb.UsbEndpoint;
import javax.usb.UsbException;
import javax.usb.UsbPipe;
import java.util.List;

public class CloseableUsbPipe implements AutoCloseable
{
    private static final org.slf4j.Logger logger = LoggerFactory.getLogger(CloseableUsbPipe.class);

    private final UsbPipe underlying;
    private final int address;

    public CloseableUsbPipe(List<UsbEndpoint> endpoints, int address) throws UsbException
    {
        for (UsbEndpoint endpoint : endpoints)
            if ((endpoint.getUsbEndpointDescriptor().bEndpointAddress() & 0xff) == address)
            {
                UsbPipe pipe = endpoint.getUsbPipe();
                this.address = address;
                logger.atDebug().log("opening pipe for {}", address);
                pipe.open();
                underlying = pipe;
                return;
            }

        throw new FluxEngineException(String.format(
                "unable to open USB endpoint for address %d",
                address));
    }

    @Override
    public void close() throws Exception
    {
        logger.atDebug().log("closing pipe for {}", address);
        underlying.close();
    }

    UsbPipe get()
    {
        return underlying;
    }
}
