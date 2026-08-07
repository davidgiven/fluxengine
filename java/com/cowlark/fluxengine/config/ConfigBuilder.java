package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_A2R;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_AU;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_CWF;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_DMK;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_DRIVE;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_ERASE;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_FLUX;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_FLX;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_KRYOFLUX;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_SCP;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_TEST_PATTERN;
import static com.cowlark.fluxengine.config.Common.FluxSourceSinkType.FLUXTYPE_VCD;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_D64;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_D88;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_DIM;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_DISKCOPY;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_FDI;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_IMD;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_IMG;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_JV3;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_NFD;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_NSI;
import static com.cowlark.fluxengine.config.Common.ImageReaderWriterType.IMAGETYPE_TD0;

import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.Flags;
import com.cowlark.fluxengine.fluxsink.Fluxsink;
import com.cowlark.fluxengine.fluxsource.Fluxsource;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigBuilder
{
    private ConfigProto.Builder proto = ConfigProto.newBuilder()
            .setFluxSource(Fluxsource.FluxSourceProto.newBuilder().setType(FLUXTYPE_DRIVE).build());

    public ConfigBuilder()
    {
    }

    public ConfigBuilder fromFlags(ImmutableList<String> args, FlagGroup... group)
    {
        ImmutableList<FlagGroup> allGroups = ImmutableList.<FlagGroup>builder()
                .add(group)
                .add(new ConfigFlagGroup(this))
                .build();
        Flags.parse(args, allGroups);

        return this;
    }

    public ConfigBuilder loadConfigFile(String name)
    {
        String contents;
        try
        {
            contents = Files.readString(Path.of(name));
        } catch (IOException e)
        {
            throw new ConfigException("Cannot open '" + name + "': " + e.getMessage());
        }

        try
        {
            TextFormat.merge(contents, proto);
        } catch (TextFormat.ParseException e)
        {
            throw new ConfigException("couldn't load external config proto");
        }

        return this;
    }

    public ConfigBuilder withFluxSource(String filename)
    {
        Fluxsource.FluxSourceProto.Builder fluxSource = proto.getFluxSourceBuilder();
        if (filename.endsWith(".flux"))
        {
            fluxSource.setType(FLUXTYPE_FLUX);
            fluxSource.getFl2Builder().setFilename(filename);
        } else if (filename.endsWith(".scp"))
        {
            fluxSource.setType(FLUXTYPE_SCP);
            fluxSource.getScpBuilder().setFilename(filename);
        } else if (filename.endsWith(".a2r"))
        {
            fluxSource.setType(FLUXTYPE_A2R);
            fluxSource.getA2RBuilder().setFilename(filename);
        } else if (filename.endsWith(".cwf"))
        {
            fluxSource.setType(FLUXTYPE_CWF);
            fluxSource.getCwfBuilder().setFilename(filename);
        } else if (filename.startsWith("dmk:"))
        {
            fluxSource.setType(FLUXTYPE_DMK);
            fluxSource.getDmkBuilder().setDirectory(filename.substring(4));
        } else if (filename.equals("erase:"))
        {
            fluxSource.setType(FLUXTYPE_ERASE);
        } else if (filename.startsWith("kryoflux:"))
        {
            fluxSource.setType(FLUXTYPE_KRYOFLUX);
            fluxSource.getKryofluxBuilder().setDirectory(filename.substring(9));
        } else if (filename.startsWith("testpattern:"))
        {
            fluxSource.setType(FLUXTYPE_TEST_PATTERN);
        } else if (filename.startsWith("drive:"))
        {
            fluxSource.setType(FLUXTYPE_DRIVE);
            proto.getDriveBuilder().setDrive(Integer.parseInt(filename.substring(6)));
        } else if (filename.startsWith("flx:"))
        {
            fluxSource.setType(FLUXTYPE_FLX);
            fluxSource.getFlxBuilder().setDirectory(filename.substring(4));
        } else
            throw new ConfigException("unrecognised flux filename '" + filename + "'");
        return this;
    }

    public ConfigBuilder withCopyFluxTo(String filename)
    {
        setFluxSink(proto.getDecoderBuilder().getCopyFluxToBuilder(), filename);
        return this;
    }

    public ConfigBuilder withFluxSink(String filename)
    {
        setFluxSink(proto.getFluxSinkBuilder(), filename);
        return this;
    }

    private void setFluxSink(Fluxsink.FluxSinkProto.Builder fluxSink, String filename)
    {
        if (filename.endsWith(".flux"))
        {
            fluxSink.setType(FLUXTYPE_FLUX);
            fluxSink.getFl2Builder().setFilename(filename);
        } else if (filename.endsWith(".scp"))
        {
            fluxSink.setType(FLUXTYPE_SCP);
            fluxSink.getScpBuilder().setFilename(filename);
        } else if (filename.endsWith(".a2r"))
        {
            fluxSink.setType(FLUXTYPE_A2R);
            fluxSink.getA2RBuilder().setFilename(filename);
        } else if (filename.startsWith("drive:"))
        {
            fluxSink.setType(FLUXTYPE_DRIVE);
            proto.getDriveBuilder().setDrive(Integer.parseInt(filename.substring(6)));
        } else if (filename.startsWith("vcd:"))
        {
            fluxSink.setType(FLUXTYPE_VCD);
            fluxSink.getVcdBuilder().setDirectory(filename.substring(4));
        } else if (filename.startsWith("au:"))
        {
            fluxSink.setType(FLUXTYPE_AU);
            fluxSink.getAuBuilder().setDirectory(filename.substring(3));
        } else
            throw new ConfigException("unrecognised flux filename '" + filename + "'");
    }

    public ConfigBuilder withImageWriter(String filename)
    {
        Common.ImageReaderWriterType type = imageType(filename);
        if (type == null || isReadOnlyImage(filename))
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        proto.getImageWriterBuilder().setType(type).setFilename(filename);
        return this;
    }

    public ConfigBuilder withImageReader(String filename)
    {
        Common.ImageReaderWriterType type = imageType(filename);
        if (type == null)
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        proto.getImageReaderBuilder().setType(type).setFilename(filename);
        return this;
    }

    private static Common.ImageReaderWriterType imageType(String filename)
    {
        if (filename.endsWith(".adf") || filename.endsWith(".d81") || filename.endsWith(".dsk")
                || filename.endsWith(".img") || filename.endsWith(".st") || filename.endsWith(".vgi")
                || filename.endsWith(".xdf"))
            return IMAGETYPE_IMG;
        else if (filename.endsWith(".d64"))
            return IMAGETYPE_D64;
        else if (filename.endsWith(".d88"))
            return IMAGETYPE_D88;
        else if (filename.endsWith(".dim"))
            return IMAGETYPE_DIM;
        else if (filename.endsWith(".diskcopy"))
            return IMAGETYPE_DISKCOPY;
        else if (filename.endsWith(".fdi"))
            return IMAGETYPE_FDI;
        else if (filename.endsWith(".imd"))
            return IMAGETYPE_IMD;
        else if (filename.endsWith(".jv3"))
            return IMAGETYPE_JV3;
        else if (filename.endsWith(".nfd"))
            return IMAGETYPE_NFD;
        else if (filename.endsWith(".nsi"))
            return IMAGETYPE_NSI;
        else if (filename.endsWith(".td0"))
            return IMAGETYPE_TD0;
        else
            return null;
    }

    private static boolean isReadOnlyImage(String filename)
    {
        return filename.endsWith(".dim") || filename.endsWith(".fdi") || filename.endsWith(".jv3")
                || filename.endsWith(".nfd") || filename.endsWith(".td0");
    }

    public ConfigBuilder showCurrentConfig()
    {
        return this;
    }

    public ConfigBuilder set(String key, String value)
    {
        ProtoPath.set(proto, key, value);
        return this;
    }

    public ConfigProto build()
    {
        validate();
        return proto.build();
    }

    private void validate()
    {
        if ((proto.getFluxSource().getType() == FLUXTYPE_DRIVE) ||
                (proto.getFluxSink().getType() == FLUXTYPE_DRIVE))
            validateUsb();
    }

    private void validateUsb()
    {
        if (!proto.getUsb().hasSerial())
            proto.getUsbBuilder().setSerial(UsbFinder.selectDevice(proto).serial);
    }

}
