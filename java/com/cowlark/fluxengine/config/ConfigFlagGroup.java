package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.Flag;
import com.cowlark.fluxengine.core.flags.FlagGroup;

public class ConfigFlagGroup extends FlagGroup
{
    private final ConfigBuilder builder;

    public ConfigFlagGroup(ConfigBuilder builder)
    {
        this.builder = builder;

        ActionFlag.builder()
                .setGroup(this)
                .setName("-c")
                .setName("--config")
                .setHelpText("Reads an internal or external configuration file.")
                .setValueCallback(builder::loadConfigFile)
                .build();
        ActionFlag.builder()
                .setGroup(this)
                .setName("--show-config")
                .setHelpText("Shows the currently set configuration and halts.")
                .setVoidCallback(builder::showCurrentConfig)
                .build();
    }

    @Override
    public Flag findFlag(String key)
    {
        if (key.startsWith("--"))
        {
            String path = key.substring(2);
            if (key.contains("."))
            {
                /* Dots: setting a config key. */
                return ActionFlag.builder()
                        .setGroup(this)
                        .setValueCallback(value -> builder.set(path, value))
                        .build();
            } else
            {
                /* No dots: this is an option name; look it up (throws if
                 * unknown). */
                ConfigBuilder.OptionInfo option = builder.findOption(path);
                if (option.usesValue())
                    return ActionFlag.builder()
                            .setGroup(this)
                            .setValueCallback(arg -> builder.applyOption(option, arg))
                            .build();
                else
                    return ActionFlag.builder()
                            .setGroup(this)
                            .setVoidCallback(() -> builder.applyOption(option, null))
                            .build();
            }
        }
        return super.findFlag(key);
    }
}
