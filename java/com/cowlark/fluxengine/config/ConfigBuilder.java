package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_A2R;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_AU;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_CWF;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DMK;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_ERASE;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_FLUX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_FLX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_KRYOFLUX;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_SCP;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_TEST_PATTERN;
import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_VCD;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_D64;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_D88;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_DIM;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_DISKCOPY;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_FDI;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_IMD;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_IMG;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_JV3;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_NFD;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_NSI;
import static com.cowlark.fluxengine.config.ImageReaderWriterType.IMAGETYPE_TD0;

import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.Flags;
import com.cowlark.fluxengine.data.Formats;
import com.cowlark.fluxengine.fluxsink.FluxSinkProto;
import com.cowlark.fluxengine.fluxsource.FluxSourceProto;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.TextFormat;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigBuilder
{
    /* The groups which have had an option applied, so that applyDefaultOptions
     * knows not to apply their defaults. */
    private final Set<OptionGroupProto> appliedOptions = new HashSet<>();
    private ConfigProto.Builder proto = Formats.get("_global_options").toBuilder();

    public ConfigBuilder()
    {
    }

    private static ImageReaderWriterType imageType(String filename)
    {
        if (filename.endsWith(".adf") || filename.endsWith(".d81") || filename.endsWith(".dsk") ||
                filename.endsWith(".img") || filename.endsWith(".st") ||
                filename.endsWith(".vgi") || filename.endsWith(".xdf"))
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
        return filename.endsWith(".dim") || filename.endsWith(".fdi") ||
                filename.endsWith(".jv3") || filename.endsWith(".nfd") || filename.endsWith(".td0");
    }

    /* Quotes a string if it contains spaces or quote characters, ported from
     * lib/core/utils.cc quote(). */
    private static String quote(String s)
    {
        boolean spaces = s.contains(" ");
        if (!spaces && !s.contains("\\") && !s.contains("'") && !s.contains("\""))
            return s;

        StringBuilder ss = new StringBuilder();
        if (spaces)
            ss.append('"');

        for (int i = 0; i < s.length(); i++)
        {
            char c = s.charAt(i);
            if ((c == '\\') || (c == '"') || (c == '!'))
                ss.append('\\');
            ss.append(c);
        }

        if (spaces)
            ss.append('"');

        return ss.toString();
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
        /* Try to load the config from the built-in formats first. */

        ConfigProto config = Formats.get(name);
        if (config != null)
        {
            proto.mergeFrom(config);
            return this;
        }

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

    public ConfigBuilder mergeConfig(ConfigProto other)
    {
        proto.mergeFrom(other);
        return this;
    }

    public ConfigBuilder withFluxSource(String filename)
    {
        FluxSourceProto.Builder fluxSource = proto.getFluxSourceBuilder();
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

    private void setFluxSink(FluxSinkProto.Builder fluxSink, String filename)
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
        ImageReaderWriterType type = imageType(filename);
        if (type == null || isReadOnlyImage(filename))
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        proto.getImageWriterBuilder().setType(type).setFilename(filename);
        return this;
    }

    public ConfigBuilder withImageReader(String filename)
    {
        ImageReaderWriterType type = imageType(filename);
        if (type == null)
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        proto.getImageReaderBuilder().setType(type).setFilename(filename);
        return this;
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

    /* Looks up an option by name, ported from Config::findOption. The group
     * value parameter of the C++ version is not needed here, so it takes a
     * key only. */
    public OptionInfo findOption(String name)
    {
        /* First look for any individual options. */

        for (OptionProto option : proto.getOptionList())
        {
            if (name.equals(option.getName()))
                return new OptionInfo(null, option, false);
        }

        /* Now search for individual options in unnamed groups. */

        for (OptionGroupProto optionGroup : proto.getOptionGroupList())
        {
            if (optionGroup.getName().isEmpty())
            {
                for (OptionProto option : optionGroup.getOptionList())
                {
                    if (name.equals(option.getName()))
                        return new OptionInfo(optionGroup, option, false);
                }
            }
        }

        /* Now look for named groups. A group itself is not an option; it is
         * selected by supplying a value, so usesValue is true. */

        for (OptionGroupProto optionGroup : proto.getOptionGroupList())
        {
            if (name.equals(optionGroup.getName()))
                return new OptionInfo(optionGroup, null, true);
        }

        throw new ConfigException(String.format("option %s not found", name));
    }

    public void applyOption(OptionInfo option, String value)
    {
        OptionProto optionProto = option.option();
        if ((optionProto == null) && option.usesValue())
        {
            /* A group with no option set means we need to select the option by
             * value. */

            for (OptionProto candidate : option.group().getOptionList())
            {
                if (value.equals(candidate.getName()))
                {
                    optionProto = candidate;
                    break;
                }
            }

            if (optionProto == null)
                throw new InapplicableOptionException(
                        "value %s is not valid for option %s; valid values are: %s",
                        value,
                        option.group().getName(),
                        option.group()
                                .getOptionList()
                                .stream()
                                .map(OptionProto::getName)
                                .collect(java.util.stream.Collectors.joining(", ")));
        }

        checkOptionValid(optionProto);
        if (option.group() != null)
            appliedOptions.add(option.group());
        Logger.log(new OptionLogMessage("user option", optionProto));
        proto.mergeFrom(optionProto.getConfig());
    }

    /* Applies the default option for every group which doesn't have one set,
     * ported from Config::applyDefaultOptions. */
    private void applyDefaultOptions()
    {
        for (OptionGroupProto group : proto.getOptionGroupList())
        {
            if (!appliedOptions.contains(group))
            {
                for (OptionProto optionProto : group.getOptionList())
                {
                    if (optionProto.getSetByDefault())
                    {
                        checkOptionValid(optionProto);
                        appliedOptions.add(group);

                        /* Default options should never override anything the user set. */
                        Logger.log(new OptionLogMessage("default option", optionProto));
                        proto = optionProto.getConfig().toBuilder().mergeFrom(proto.build());
                    }
                }
            }
        }
    }

    private void checkOptionValid(OptionProto optionProto)
    {
        for (OptionPrerequisiteProto req : optionProto.getPrerequisiteList())
        {
            boolean matched = false;
            try
            {
                String value = ProtoPath.get(proto, req.getKey());
                for (String requiredValue : req.getValueList())
                    matched |= requiredValue.equals(value);
            } catch (ProtoPathNotFoundException e)
            {
                /* This field isn't available, therefore it cannot match. */
            }

            if (!matched)
            {
                StringBuilder ss = new StringBuilder();
                ss.append('[');
                boolean first = true;
                for (String requiredValue : req.getValueList())
                {
                    if (!first)
                        ss.append(", ");
                    ss.append(quote(requiredValue));
                    first = false;
                }
                ss.append(']');

                throw new InapplicableOptionException(
                        "option '%s' is inapplicable to this configuration " +
                                "because %s=%s could not be met",
                        optionProto.getName(),
                        req.getKey(),
                        ss.toString());
            }
        }
    }

    public ConfigProto build()
    {
        applyDefaultOptions();
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

    /* The result of looking up an option, ported from
     * lib/config/config.h Config::OptionInfo. */
    public record OptionInfo(OptionGroupProto group, OptionProto option, boolean usesValue)
    {
    }

}
