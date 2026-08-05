package com.cowlark.fluxengine.config;

import com.cowlark.fluxengine.config.ConfigFile.ConfigProto;
import com.google.common.collect.ImmutableList;

/**
 * The assembled configuration, built from the unmatched command-line
 * arguments.
 */
public class ConfigParser
{
    private ConfigProto proto = ConfigProto.getDefaultInstance();

    public ConfigParser()
    {
    }

    public ConfigParser parse(ImmutableList<String> args)
    {
        int i = 0;
        while (i < args.size())
        {
            String arg = args.get(i);
            if (arg.startsWith("--"))
            {
                int eq = arg.indexOf('=');
                if (eq >= 0)
                {
                    set(arg.substring(2, eq), arg.substring(eq + 1));
                }
                else if (i + 1 < args.size())
                {
                    set(arg.substring(2), args.get(i + 1));
                    i++;
                }
            }
            else if (arg.startsWith("-") && arg.length() > 1)
            {
                int eq = arg.indexOf('=');
                if (eq >= 0)
                {
                    set(arg.substring(1, eq), arg.substring(eq + 1));
                }
                else if (i + 1 < args.size())
                {
                    set(arg.substring(1), args.get(i + 1));
                    i++;
                }
            }
            /* bare arguments are ignored */
            i++;
        }
        return this;
    }

    public ConfigParser set(String key, String value)
    {
        return this;
    }

    public ConfigProto build()
    {
        return proto;
    }

}
