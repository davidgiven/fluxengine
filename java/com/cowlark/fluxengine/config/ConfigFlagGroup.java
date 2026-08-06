package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.Flag;
import com.cowlark.fluxengine.core.flags.FlagGroup;

public class ConfigFlagGroup extends FlagGroup
{
    public ConfigFlagGroup()
    {
        ActionFlag.builder()
                .setGroup(this)
                .setName("-c")
                .setName("--config")
                .setHelpText("Reads an internal or external configuration file.")
                .build();
        ActionFlag.builder()
                .setGroup(this)
                .setName("--show-config")
                .setHelpText("Shows the currently set configuration and halts.")
                .build();
    }

    @Override
    public Flag findFlag(String key)
    {
        if (key.contains("."))
            return ActionFlag.builder().build();
        return null;
    }
}
