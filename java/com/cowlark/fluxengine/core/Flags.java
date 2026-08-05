package com.cowlark.fluxengine.core;

import java.util.ArrayList;
import java.util.HashMap;
import java.util.List;
import java.util.Map;
import java.util.function.Consumer;
import java.util.function.Predicate;

import lombok.Builder;

/**
 * Command-line flags system, ported from lib/config/flags.h.
 */
public class Flags
{
    public static class FlagGroup
    {
        private final List<FlagGroup> parents;
        private final List<Flag> flags = new ArrayList<>();
        private boolean initialised;

        public FlagGroup()
        {
            parents = List.of();
        }

        public FlagGroup(FlagGroup... parents)
        {
            this.parents = List.of(parents);
        }

        public void addFlag(Flag flag)
        {
            flags.add(flag);
        }

        public void parse(String[] argv)
        {
            List<String> filenames = parseWithFilenames(argv, unused -> false);
            if (!filenames.isEmpty())
                throw new FluxEngineException(
                    "non-option parameter '" + filenames.get(0) + "' seen (try --help)");
        }

        public List<String> parseWithFilenames(String[] argv, Predicate<String> callback)
        {
            if (initialised)
                throw new IllegalStateException("called parse() twice");

            /* Recursively accumulate a list of all flags. */
            Map<String, Flag> flagsByName = new HashMap<>();
            recurse(this, flagsByName);

            List<String> filenames = new ArrayList<>();
            int index = 0;
            while (index < argv.length)
            {
                String thisArg = argv[index];
                String thatArg = (index < argv.length - 1) ? argv[index + 1] : "";

                String key;
                String value;
                boolean useThat = false;

                if (thisArg.isEmpty())
                {
                    /* Ignore this argument. */
                }
                else if (thisArg.charAt(0) != '-')
                {
                    /* This is a filename. */
                    if (!callback.test(thisArg))
                        filenames.add(thisArg);
                }
                else
                {
                    if (thisArg.length() > 1 && thisArg.charAt(1) == '-')
                    {
                        /* Long option. */
                        int equals = thisArg.lastIndexOf('=');
                        if (equals >= 0)
                        {
                            key = thisArg.substring(0, equals);
                            value = thisArg.substring(equals + 1);
                        }
                        else
                        {
                            key = thisArg;
                            value = thatArg;
                            useThat = true;
                        }
                    }
                    else
                    {
                        /* Short option. */
                        if (thisArg.length() > 2)
                        {
                            key = thisArg.substring(0, 2);
                            value = thisArg.substring(2);
                        }
                        else
                        {
                            key = thisArg;
                            value = thatArg;
                            useThat = true;
                        }
                    }

                    Flag flag = flagsByName.get(key);
                    if (flag == null)
                        throw new FluxEngineException(
                            "unrecognised flag '" + key + "'; try --help");
                    flag.set(value);
                    if (useThat && flag.hasArgument())
                        index++;
                }

                index++;
            }

            return filenames;
        }

        public void checkInitialised()
        {
            if (!initialised)
                throw new IllegalStateException("Attempt to access uninitialised flag");
        }

        private void recurse(FlagGroup group, Map<String, Flag> flagsByName)
        {
            if (group.initialised)
                return;

            for (FlagGroup parent : group.parents)
                recurse(parent, flagsByName);

            for (Flag flag : group.flags)
            {
                for (String name : flag.names())
                {
                    if (flagsByName.containsKey(name))
                        throw new IllegalStateException(
                            "two flags use the name '" + name + "'");
                    flagsByName.put(name, flag);
                }
            }

            group.initialised = true;
        }
    }

    public abstract static class Flag
    {
        private final FlagGroup group;
        private final List<String> names;
        private final String helptext;

        protected Flag(FlagGroup group, List<String> names, String helptext)
        {
            this.group = group;
            this.names = List.copyOf(names);
            this.helptext = helptext;
            group.addFlag(this);
        }

        public String name()
        {
            return names.get(0);
        }

        public List<String> names()
        {
            return names;
        }

        public String helptext()
        {
            return helptext;
        }

        public abstract boolean hasArgument();

        public abstract String defaultValueAsString();

        public abstract void set(String value);

        protected void checkInitialised()
        {
            group.checkInitialised();
        }
    }

    public static class ActionFlag extends Flag
    {
        private final Runnable voidCallback;
        private final Consumer<String> valueCallback;
        private final boolean hasArgument;

        @Builder(setterPrefix = "set")
        private ActionFlag(FlagGroup group, List<String> names, String helpText,
            Runnable voidCallback, Consumer<String> valueCallback)
        {
            super(group, names, helpText);
            this.voidCallback = voidCallback;
            this.valueCallback = valueCallback;
            hasArgument = valueCallback != null;
        }

        @Override
        public boolean hasArgument()
        {
            return hasArgument;
        }

        @Override
        public String defaultValueAsString()
        {
            return "";
        }

        @Override
        public void set(String value)
        {
            if (hasArgument)
                valueCallback.accept(value);
            else
                voidCallback.run();
        }
    }

    public static class SettableFlag extends Flag
    {
        private boolean value;

        @Builder(setterPrefix = "set")
        private SettableFlag(FlagGroup group, List<String> names, String helpText)
        {
            super(group, names, helpText);
        }

        public boolean get()
        {
            checkInitialised();
            return value;
        }

        @Override
        public boolean hasArgument()
        {
            return false;
        }

        @Override
        public String defaultValueAsString()
        {
            return "false";
        }

        @Override
        public void set(String value)
        {
            this.value = true;
        }
    }

    public abstract static class ValueFlag<T> extends Flag
    {
        private T defaultValue;
        private final Consumer<T> callback;
        protected T value;
        private boolean isSet;

        protected ValueFlag(FlagGroup group, List<String> names, String helptext,
            T defaultValue, Consumer<T> callback)
        {
            super(group, names, helptext);
            this.defaultValue = defaultValue;
            this.value = defaultValue;
            this.callback = callback;
        }

        public T get()
        {
            checkInitialised();
            return value;
        }

        public boolean isSet()
        {
            return isSet;
        }

        public void setDefaultValue(T value)
        {
            defaultValue = value;
            this.value = value;
        }

        protected void setValue(T value)
        {
            this.value = value;
            callback.accept(value);
            isSet = true;
        }
    }

    public static class StringFlag extends ValueFlag<String>
    {
        @Builder(setterPrefix = "set")
        private StringFlag(FlagGroup group, List<String> names, String helpText,
            String defaultValue, Consumer<String> callback)
        {
            super(group, names, helpText,
                defaultValue != null ? defaultValue : "",
                callback != null ? callback : unused -> {});
        }

        @Override
        public boolean hasArgument()
        {
            return true;
        }

        @Override
        public String defaultValueAsString()
        {
            return value;
        }

        @Override
        public void set(String value)
        {
            setValue(value);
        }
    }

    public static class IntFlag extends ValueFlag<Integer>
    {
        @Builder(setterPrefix = "set")
        private IntFlag(FlagGroup group, List<String> names, String helpText,
            int defaultValue, Consumer<Integer> callback)
        {
            super(group, names, helpText, defaultValue,
                callback != null ? callback : unused -> {});
        }

        @Override
        public boolean hasArgument()
        {
            return true;
        }

        @Override
        public String defaultValueAsString()
        {
            return Integer.toString(value);
        }

        @Override
        public void set(String value)
        {
            setValue(Integer.parseInt(value));
        }
    }

    public static class HexIntFlag extends ValueFlag<Integer>
    {
        @Builder(setterPrefix = "set")
        private HexIntFlag(FlagGroup group, List<String> names, String helpText,
            Integer defaultValue)
        {
            super(group, names, helpText,
                defaultValue != null ? defaultValue : 0, unused -> {});
        }

        @Override
        public boolean hasArgument()
        {
            return true;
        }

        @Override
        public String defaultValueAsString()
        {
            return String.format("0x%x", value);
        }

        @Override
        public void set(String value)
        {
            setValue(Integer.parseInt(value));
        }
    }

    public static class DoubleFlag extends ValueFlag<Double>
    {
        @Builder(setterPrefix = "set")
        private DoubleFlag(FlagGroup group, List<String> names, String helpText,
            Double defaultValue, Consumer<Double> callback)
        {
            super(group, names, helpText,
                defaultValue != null ? defaultValue : 1.0,
                callback != null ? callback : unused -> {});
        }

        @Override
        public boolean hasArgument()
        {
            return true;
        }

        @Override
        public String defaultValueAsString()
        {
            return Double.toString(value);
        }

        @Override
        public void set(String value)
        {
            setValue(Double.parseDouble(value));
        }
    }

    public static class BoolFlag extends ValueFlag<Boolean>
    {
        @Builder(setterPrefix = "set")
        private BoolFlag(FlagGroup group, List<String> names, String helpText,
            boolean defaultValue, Consumer<Boolean> callback)
        {
            super(group, names, helpText, defaultValue,
                callback != null ? callback : unused -> {});
        }

        @Override
        public boolean hasArgument()
        {
            return true;
        }

        @Override
        public String defaultValueAsString()
        {
            return value ? "true" : "false";
        }

        @Override
        public void set(String value)
        {
            if (value.equals("true") || value.equals("y"))
                setValue(true);
            else if (value.equals("false") || value.equals("n"))
                setValue(false);
            else
                throw new FluxEngineException(
                    "can't parse '" + value + "'; try 'true' or 'false'");
        }
    }

    private Flags()
    {
    }
}
