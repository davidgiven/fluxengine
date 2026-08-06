package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.core.flags.ActionFlag;
import com.cowlark.fluxengine.core.flags.Flag;
import com.cowlark.fluxengine.core.flags.FlagGroup;

public class ConfigFlagGroup extends FlagGroup
{
    public ConfigFlagGroup()
    {
        addFlag(ActionFlag.builder()
                .setName("-c")
                .setName("--config")
                .build());
        addFlag(ActionFlag.builder()
                .setName("--show-config")
                .build());
    }

    @Override
    public Flag findFlag(String key)
    {
        if (key.contains("."))
            return ActionFlag.builder().build();
        return null;
    }
}
