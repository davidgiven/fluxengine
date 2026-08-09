package com.cowlark.fluxengine.algorithms;

import com.cowlark.fluxengine.arch.Arch;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.LogMessage;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.SupplierOfAutocloseable;
import com.cowlark.fluxengine.data.DiskLayout;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.fluxsink.FluxSinkFactory;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.imagereader.ImageReader;
import com.cowlark.fluxengine.imagewriter.ImageWriter;
import com.cowlark.fluxengine.usb.UsbDevice;
import com.cowlark.fluxengine.usb.UsbFactory;
import com.google.common.base.Supplier;
import com.google.common.base.Suppliers;

public abstract class Operation implements AutoCloseable
{
    private final ConfigProto configProto;
    private double diskRotationalPeriodNs;
    private Supplier<DiskLayout> diskLayoutSupplier;
    private SupplierOfAutocloseable<FluxSource> fluxSourceSupplier;
    private SupplierOfAutocloseable<FluxSinkFactory> fluxSinkFactorySupplier;
    private SupplierOfAutocloseable<UsbDevice> usbDeviceSupplier;
    private Supplier<Decoder> decoderSupplier;
    private Supplier<Encoder> encoderSupplier;
    private SupplierOfAutocloseable<ImageReader> imageReaderSupplier;
    private SupplierOfAutocloseable<ImageWriter> imageWriterSupplier;

    public Operation(ConfigProto configProto)
    {
        this.configProto = configProto;
        diskLayoutSupplier = Suppliers.memoize(() -> new DiskLayout(configProto));
        fluxSourceSupplier = new SupplierOfAutocloseable(() -> FluxSource.create(configProto));
        fluxSinkFactorySupplier =
                new SupplierOfAutocloseable(() -> FluxSinkFactory.create(configProto));
        usbDeviceSupplier = new SupplierOfAutocloseable(() -> UsbFactory.connect(configProto));
        decoderSupplier = Suppliers.memoize(() -> Arch.createDecoder(configProto));
        encoderSupplier = Suppliers.memoize(() -> Arch.createEncoder(configProto));
        imageWriterSupplier = new SupplierOfAutocloseable(() -> ImageWriter.create(configProto));
        imageReaderSupplier = new SupplierOfAutocloseable(() -> ImageReader.create(configProto));
    }

    @Override
    public void close() throws Exception
    {
        fluxSourceSupplier.close();
        fluxSinkFactorySupplier.close();
        usbDeviceSupplier.close();
        imageWriterSupplier.close();
        imageReaderSupplier.close();
    }

    public ConfigProto getConfig()
    {
        return configProto;
    }

    public DiskLayout getDiskLayout()
    {
        return diskLayoutSupplier.get();
    }

    public FluxSource getFluxSource()
    {
        return fluxSourceSupplier.get();
    }

    public FluxSinkFactory getFluxSinkFactory()
    {
        return fluxSinkFactorySupplier.get();
    }

    public Decoder getDecoder()
    {
        return decoderSupplier.get();
    }

    public Encoder getEncoder()
    {
        return encoderSupplier.get();
    }

    public ImageReader getImageReader()
    {
        return imageReaderSupplier.get();
    }

    public ImageWriter getImageWriter()
    {
        return imageWriterSupplier.get();
    }

    public double getDiskRotationalPeriodNs()
    {
        if (diskRotationalPeriodNs != 0)
            return diskRotationalPeriodNs;
        diskRotationalPeriodNs = configProto.getDrive().getRotationalPeriodMs() * 1e6;
        if (diskRotationalPeriodNs == 0)
        {
            UsbDevice device = UsbFactory.reconnect(configProto);

            Logger.log(new LogMessage.BeginOperationLogMessage("Measuring drive rotational speed"));
            Logger.log(new LogMessage.BeginSpeedOperationLogMessage());

            int retries = 5;
            do
            {
                diskRotationalPeriodNs =
                        device.getRotationalPeriod(configProto.getDrive().getHardSectorCount());
                retries--;
            } while ((diskRotationalPeriodNs == 0) && (retries > 0));
            Logger.log(new LogMessage.EndOperationLogMessage(""));
        }

        if (diskRotationalPeriodNs == 0)
            throw new FluxEngineException("Failed\nIs a disk in the drive?");

        Logger.log(new LogMessage.EndSpeedOperationLogMessage(diskRotationalPeriodNs));
        return diskRotationalPeriodNs;
    }

    void adjustTrackOnError(int baseTrack)
    {
        switch (getConfig().getDrive().getErrorBehaviour())
        {
            case NOTHING:
                break;

            case RECALIBRATE:
                getFluxSource().recalibrate();
                break;

            case JIGGLE:
                if (baseTrack > 0)
                    getFluxSource().seek(baseTrack - 1);
                else
                    getFluxSource().seek(baseTrack + 1);
                break;
        }
    }
}
