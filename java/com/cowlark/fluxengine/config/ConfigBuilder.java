package com.cowlark.fluxengine.config;

import static com.cowlark.fluxengine.config.FluxSourceSinkType.FLUXTYPE_DRIVE;
import static com.cowlark.fluxengine.config.ImageFormats.Mode.MODE_RO;

import com.cowlark.fluxengine.config.FluxFormats.FluxFormat;
import com.cowlark.fluxengine.config.ImageFormats.ImageFormat;
import com.cowlark.fluxengine.core.FluxEngineException;
import com.cowlark.fluxengine.core.Logger;
import com.cowlark.fluxengine.core.flags.FlagGroup;
import com.cowlark.fluxengine.core.flags.Flags;
import com.cowlark.fluxengine.data.Formats;
import com.cowlark.fluxengine.fluxsource.FluxSource;
import com.cowlark.fluxengine.imagereader.ImageReader;
import com.cowlark.fluxengine.usb.UsbFinder;
import com.google.common.collect.ImmutableList;
import com.google.protobuf.TextFormat;
import lombok.SneakyThrows;
import java.io.IOException;
import java.nio.file.Files;
import java.nio.file.Path;
import java.util.HashSet;
import java.util.Set;
import java.util.regex.Matcher;

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
        ImmutableList<FlagGroup> allGroups = ImmutableList
                .<FlagGroup>builder()
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

    private static FluxFormat findFluxFormat(String filename)
    {
        for (FluxFormat format : FluxFormats.formats)
        {
            if (format.matcher().matcher(filename).matches())
                return format;
        }
        throw new ConfigException("unrecognised flux filename '" + filename + "'");
    }


    @SneakyThrows
    public ConfigBuilder withFluxSource(String filename)
    {
        FluxFormat format = findFluxFormat(filename);

        Matcher matcher = format.matcher().matcher(filename);
        matcher.matches();
        format.sourceBuilder().accept(matcher.group(1), proto);

        /* If the FluxSource has any extra config to contribute, add it here. */

        try (FluxSource fluxSource = FluxSource.create(
                ConfigProto
                        .newBuilder()
                        .setFluxSource(proto.getFluxSource())
                        .build(), () -> null))
        {
            ConfigProto extraConfig = fluxSource.getExtraConfig();
            if (extraConfig != null)
            {
                /* This merge is backwards so that options set on the command line take precedence
                 * over what's in the image. */
                proto = extraConfig.toBuilder().mergeFrom(proto.build());
            }
        } catch (FluxEngineException e)
        {
            /* File not found --- ignore. */
        }
        return this;
    }

    public ConfigBuilder withCopyFluxTo(String filename)
    {
        ConfigProto.Builder builder = ConfigProto.newBuilder();

        FluxFormat format = findFluxFormat(filename);

        Matcher matcher = format.matcher().matcher(filename);
        matcher.matches();

        format.sinkBuilder().accept(matcher.group(1), builder);
        if (builder.getFluxSink().getType() == FLUXTYPE_DRIVE)
            throw new ConfigException("you can't copy flux to a hardware device");

        proto.getDecoderBuilder().setCopyFluxTo(builder.getFluxSink());
        return this;
    }

    public ConfigBuilder withFluxSink(String filename)
    {
        FluxFormat format = findFluxFormat(filename);

        Matcher matcher = format.matcher().matcher(filename);
        matcher.matches();
        format.sinkBuilder().accept(matcher.group(1), proto);
        return this;
    }

    private ImageFormat findImageFormat(String filename)
    {
        for (ImageFormat format : ImageFormats.imageFormats)
        {
            if (filename.endsWith(format.extension()))
                return format;
        }
        return null;
    }

    public ConfigBuilder withImageWriter(String filename)
    {
        ImageFormat format = findImageFormat(filename);
        if (format == null)
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        if (format.mode() == MODE_RO)
            throw new ConfigException("image filename '" + filename + "' can only be read");
        proto.getImageWriterBuilder().setType(format.type()).setFilename(filename);
        return this;
    }

    @SneakyThrows
    public ConfigBuilder withImageReader(String filename)
    {
        ImageFormat format = findImageFormat(filename);
        if (format == null)
            throw new ConfigException("unrecognised image filename '" + filename + "'");
        proto.getImageReaderBuilder().setType(format.type()).setFilename(filename);

        try (ImageReader reader = ImageReader.create(proto.getImageReader()))
        {
            ConfigProto extraConfig = reader.getExtraConfig();
            if (extraConfig != null)
            {
                /* This merge is backwards so that options set on the command line take precedence
                 * over what's in the image. */
                proto = extraConfig.toBuilder().mergeFrom(proto.build());
            }
        }
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

    /* Returns the value of the config key at the given path, or throws a
     * ProtoPathNotFoundException if it isn't a real config field, ported from
     * Config::get. */
    public String get(String key)
    {
        return ProtoPath.get(proto, key);
    }

    /* Looks up an option by name, ported from Config::findOption. The group
     * value parameter of the C++ version is not needed here, so it takes a
     * key only. */
    public OptionInfo findOption(String name)
    {
        OptionInfo info = tryFindOption(name);
        if (info != null)
            return info;
        throw new ConfigException(String.format("option %s not found", name));
    }

    /**
     * Tries to find an option by name, returning null if not found. Shared
     * helper for {@link #findOption} and {@link ConfigFlagGroup} / {@link
     * #applyOptions} to avoid duplicate lookup logic.
     */
    public OptionInfo tryFindOption(String name)
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

        return null;
    }

    /**
     * Returns true if {@code path} is a valid config proto path (i.e. {@link
     * #get} would succeed). Used to distinguish config keys from option names.
     */
    public boolean isConfigKey(String path)
    {
        try
        {
            get(path);
            return true;
        } catch (ConfigException e)
        {
            return false;
        }
    }

    /**
     * Applies a key=value pair where the key may be an option name or a config
     * proto path. Tries option first; if not an option, falls back to config
     * path (mirrors {@link ConfigFlagGroup#findFlag} but option-first).
     */
    public ConfigBuilder applyOptionOrSet(String key, String value)
    {
        OptionInfo info = tryFindOption(key);
        if (info != null)
            return applyOption(info, value);

        // Not an option – try as config key.
        // ProtoPath.set will throw ConfigException / ProtoPathNotFoundException
        // if the path is invalid; let it propagate.
        set(key, value);
        return this;
    }

    public ConfigBuilder applyOption(String key, String value)
    {
        applyOption(findOption(key), value);
        return this;
    }

    public ConfigBuilder applyOption(OptionInfo option, String value)
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
                        option
                                .group()
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

        return this;
    }

    public ConfigBuilder applyOptions(String options)
    {
        if (options == null || options.isEmpty())
            return this;

        String[] lines = options.split("\\R", -1);
        for (int i = 0; i < lines.length; i++)
        {
            String line = lines[i];
            String trimmed = line.trim();
            if (trimmed.isEmpty())
                continue;
            if (trimmed.startsWith("#"))
                continue;

            int eq = line.indexOf('=');
            if (eq == -1)
                throw new ConfigException(String.format(
                        "parse error at line %d: '%s': missing '='",
                        i + 1,
                        line));

            String key = line.substring(0, eq).trim();
            String value = line.substring(eq + 1).trim();

            if (key.isEmpty())
                throw new ConfigException(String.format(
                        "parse error at line %d: '%s': empty key",
                        i + 1,
                        line));

            try
            {
                applyOptionOrSet(key, value);
            } catch (ConfigException e)
            {
                if (e.getMessage() != null && e.getMessage().startsWith("parse error at line"))
                    throw e;
                throw new ConfigException(
                        String.format(
                                "parse error at line %d: '%s': %s",
                                i + 1,
                                line,
                                e.getMessage()),
                        e);
            }
        }
        return this;
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
