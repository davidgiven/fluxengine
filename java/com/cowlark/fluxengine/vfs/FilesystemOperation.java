package com.cowlark.fluxengine.vfs;

import com.cowlark.fluxengine.algorithms.ReadWriteFluxOperation;
import com.cowlark.fluxengine.config.ConfigException;
import com.cowlark.fluxengine.config.FluxSourceSinkType;
import lombok.SneakyThrows;
import java.io.IOException;

public class FilesystemOperation extends ReadWriteFluxOperation
{
    private final FilesystemCaller callback;

    public FilesystemOperation(FilesystemCaller callback)
    {
        this.callback = callback;
    }

    @Override
    @SneakyThrows
    public void run()
    {
        BlockDevice blockDevice;
        if ((getConfig().getFluxSource().getType() != FluxSourceSinkType.FLUXTYPE_NOT_SET) ||
                (getConfig().getFluxSink().getType() != FluxSourceSinkType.FLUXTYPE_NOT_SET))
            blockDevice = new FluxBlockDevice(this);
        else
            blockDevice = new ImageBlockDevice(this);

        FilesystemProto fsConfig = configProto.getFilesystem();
        Filesystem filesystem = switch (fsConfig.getType())
        {
            case NOT_SET -> throw new ConfigException("no filesystem configured");
            case ACORNDFS -> new AcornDfsFilesystem(fsConfig.getAcorndfs(), blockDevice);
            case BROTHER120 -> new Brother120Filesystem(fsConfig.getBrother120(), blockDevice);
            case FATFS -> new FatFilesystem(fsConfig.getFatfs(), blockDevice);
            case CPMFS -> new CpmFilesystem(fsConfig.getCpmfs(), blockDevice);
            case AMIGAFFS -> new AmigaFilesystem(fsConfig.getAmigaffs(), blockDevice);
            case MACHFS -> new MacHfsFilesystem(fsConfig.getMachfs(), blockDevice);
            case CBMFS -> new CbmFilesystem(fsConfig.getCbmfs(), blockDevice);
            case PRODOS -> new ProdosFilesystem(fsConfig.getProdos(), blockDevice);
            case SMAKY6 -> new Smaky6Filesystem(fsConfig.getSmaky6(), blockDevice);
            case APPLEDOS -> new AppleDosFilesystem(fsConfig.getAppledos(), blockDevice);
            case PHILE -> new PhileFilesystem(fsConfig.getPhile(), blockDevice);
            case LIF -> new LifFilesystem(fsConfig.getLif(), blockDevice);
            case MICRODOS -> new MicrodosFilesystem(fsConfig.getMicrodos(), blockDevice);
            case ZDOS -> new ZDosFilesystem(fsConfig.getZdos(), blockDevice);
            case ROLAND -> new RolandFilesystem(fsConfig.getRoland(), blockDevice);
        };

        try
        {
            callback.accept(filesystem);
            filesystem.flushChanges();
        } catch (Exception e)
        {
            filesystem.discardChanges();
        }
    }

    public interface FilesystemCaller
    {
        void accept(Filesystem fs) throws IOException;
    }
}
