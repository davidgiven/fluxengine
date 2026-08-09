package com.cowlark.fluxengine.imagereader;

import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.data.Image;

/**
 * Reads sector images from disk, ported from
 * lib/imagereader/imagereader.{h,cc}.
 */
public abstract class ImageReader implements AutoCloseable
{
    protected final ImageReaderProto config;
    protected final ConfigProto fullConfig;
    protected ConfigProto extraConfig = ConfigProto.getDefaultInstance();

    public ImageReader(ImageReaderProto config)
    {
        this(config, ConfigProto.getDefaultInstance());
    }

    public ImageReader(ImageReaderProto config, ConfigProto fullConfig)
    {
        this.config = config;
        this.fullConfig = fullConfig;
    }

    public static ImageReader create(ConfigProto config)
    {
        if (!config.hasImageReader())
            throw new FluxEngineException("no image reader configured");
        return create(config, config.getImageReader());
    }

    public static ImageReader create(ImageReaderProto config)
    {
        return create(ConfigProto.getDefaultInstance(), config);
    }

    public static ImageReader create(ConfigProto fullConfig, ImageReaderProto config)
    {
        switch (config.getType())
        {
            case IMAGETYPE_DIM:
                return new DimImageReader(config, fullConfig);
            case IMAGETYPE_D88:
                return new D88ImageReader(config);
            case IMAGETYPE_FDI:
                return new FdiImageReader(config, fullConfig);
            case IMAGETYPE_IMD:
                return new ImdImageReader(config);
            case IMAGETYPE_IMG:
                return new ImgImageReader(config, fullConfig);
            case IMAGETYPE_DISKCOPY:
                return new DiskCopyImageReader(config);
            case IMAGETYPE_JV3:
                return new Jv3ImageReader(config);
            case IMAGETYPE_D64:
                return new D64ImageReader(config);
            case IMAGETYPE_NFD:
                return new NfdImageReader(config);
            case IMAGETYPE_NSI:
                return new NsiImageReader(config);
            case IMAGETYPE_TD0:
                return new Td0ImageReader(config);
            default:
                throw new FluxEngineException("bad input file config");
        }
    }

    @Override
    public void close() throws Exception
    {
    }

    /* Returns any extra config the image might want to contribute. */
    public ConfigProto getExtraConfig()
    {
        return extraConfig;
    }

    /* Reads the image. */
    public abstract Image readImage();
}
