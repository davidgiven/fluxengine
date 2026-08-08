package com.cowlark.fluxengine.arch;

import com.cowlark.fluxengine.arch.aeslanier.AesLanierDecoder;
import com.cowlark.fluxengine.arch.agat.AgatDecoder;
import com.cowlark.fluxengine.arch.agat.AgatEncoder;
import com.cowlark.fluxengine.arch.amiga.AmigaDecoder;
import com.cowlark.fluxengine.arch.amiga.AmigaEncoder;
import com.cowlark.fluxengine.arch.apple2.Apple2Decoder;
import com.cowlark.fluxengine.arch.apple2.Apple2Encoder;
import com.cowlark.fluxengine.arch.brother.BrotherDecoder;
import com.cowlark.fluxengine.arch.brother.BrotherEncoder;
import com.cowlark.fluxengine.arch.c64.Commodore64Decoder;
import com.cowlark.fluxengine.arch.c64.Commodore64Encoder;
import com.cowlark.fluxengine.arch.f85.DurangoF85Decoder;
import com.cowlark.fluxengine.arch.fb100.Fb100Decoder;
import com.cowlark.fluxengine.arch.ibm.IbmDecoder;
import com.cowlark.fluxengine.arch.ibm.IbmEncoder;
import com.cowlark.fluxengine.arch.macintosh.MacintoshDecoder;
import com.cowlark.fluxengine.arch.macintosh.MacintoshEncoder;
import com.cowlark.fluxengine.arch.micropolis.MicropolisDecoder;
import com.cowlark.fluxengine.arch.micropolis.MicropolisEncoder;
import com.cowlark.fluxengine.arch.mx.MxDecoder;
import com.cowlark.fluxengine.arch.northstar.NorthstarDecoder;
import com.cowlark.fluxengine.arch.northstar.NorthstarEncoder;
import com.cowlark.fluxengine.arch.rolandd20.RolandD20Decoder;
import com.cowlark.fluxengine.arch.smaky6.Smaky6Decoder;
import com.cowlark.fluxengine.arch.tartu.TartuDecoder;
import com.cowlark.fluxengine.arch.tartu.TartuEncoder;
import com.cowlark.fluxengine.arch.tids990.Tids990Decoder;
import com.cowlark.fluxengine.arch.tids990.Tids990Encoder;
import com.cowlark.fluxengine.arch.victor9k.Victor9kDecoder;
import com.cowlark.fluxengine.arch.victor9k.Victor9kEncoder;
import com.cowlark.fluxengine.arch.zilogmcz.ZilogMczDecoder;
import com.cowlark.fluxengine.config.ConfigProto;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.decoders.Decoder;
import com.cowlark.fluxengine.decoders.DecoderProto;
import com.cowlark.fluxengine.encoders.Encoder;
import com.cowlark.fluxengine.encoders.EncoderProto;

/**
 * The Arch class, ported from arch/arch.{h,cc}.
 */
public final class Arch
{
    private Arch()
    {
    }

    public static Decoder createDecoder(ConfigProto config)
    {
        if (!config.hasDecoder())
            throw new FluxEngineException("no decoder configured");
        return createDecoder(config.getDecoder());
    }

    public static Decoder createDecoder(DecoderProto config)
    {
        switch (config.getFormatCase())
        {
            case AGAT:
                return new AgatDecoder(config);
            case AESLANIER:
                return new AesLanierDecoder(config);
            case AMIGA:
                return new AmigaDecoder(config);
            case APPLE2:
                return new Apple2Decoder(config);
            case BROTHER:
                return new BrotherDecoder(config);
            case C64:
                return new Commodore64Decoder(config);
            case F85:
                return new DurangoF85Decoder(config);
            case FB100:
                return new Fb100Decoder(config);
            case IBM:
                return new IbmDecoder(config);
            case MACINTOSH:
                return new MacintoshDecoder(config);
            case MICROPOLIS:
                return new MicropolisDecoder(config);
            case MX:
                return new MxDecoder(config);
            case NORTHSTAR:
                return new NorthstarDecoder(config);
            case ROLANDD20:
                return new RolandD20Decoder(config);
            case SMAKY6:
                return new Smaky6Decoder(config);
            case TARTU:
                return new TartuDecoder(config);
            case TIDS990:
                return new Tids990Decoder(config);
            case VICTOR9K:
                return new Victor9kDecoder(config);
            case ZILOGMCZ:
                return new ZilogMczDecoder(config);
            default:
                throw new FluxEngineException("no decoder specified");
        }
    }

    public static Encoder createEncoder(ConfigProto config)
    {
        if (!config.hasEncoder())
            throw new FluxEngineException("no encoder configured");

        switch (config.getEncoder().getFormatCase())
        {
            case AGAT:
                return new AgatEncoder(config);
            case AMIGA:
                return new AmigaEncoder(config);
            case APPLE2:
                return new Apple2Encoder(config);
            case BROTHER:
                return new BrotherEncoder(config);
            case C64:
                return new Commodore64Encoder(config);
            case IBM:
                return new IbmEncoder(config);
            case MACINTOSH:
                return new MacintoshEncoder(config);
            case MICROPOLIS:
                return new MicropolisEncoder(config);
            case NORTHSTAR:
                return new NorthstarEncoder(config);
            case TARTU:
                return new TartuEncoder(config);
            case TIDS990:
                return new Tids990Encoder(config);
            case VICTOR9K:
                return new Victor9kEncoder(config);
            default:
                throw new FluxEngineException("no encoder specified");
        }
    }
}