#ifndef FLAGS_H
#define FLAGS_H

class DataSpec;
class Flag;
class ConfigProto;
class OptionProto;

class FlagGroup
{
public:
    FlagGroup();
    FlagGroup(std::initializer_list<FlagGroup*> groups);

public:
    void parseFlags(
        int argc,
        const char* argv[],
        std::function<bool(const std::string&)> callback =
            [](const auto&)
        {
            return false;
        });
    std::vector<std::string> parseFlagsWithFilenames(
        int argc,
        const char* argv[],
        std::function<bool(const std::string&)> callback =
            [](const auto&)
        {
            return false;
        });
    void parseFlagsWithConfigFiles(int argc,
        const char* argv[],
        const std::map<std::string, std::string>& configFiles);

    /* Load one config file (or internal config file), without expanding
     * includes. */

    static ConfigProto parseSingleConfigFile(const std::string& filename,
        const std::map<std::string, std::string>& configFiles);

    /* Load a top-level config file (or internal config file), expanding
     * includes. */

    static void parseConfigFile(const std::string& filename,
        const std::map<std::string, std::string>& configFiles);

    /* Modify the current config to engage the named option. */

    static void applyOption(const OptionProto& option);
    static bool applyOption(const std::string& option);

    void addFlag(Flag* flag);
    void checkInitialised() const;

private:
    bool _initialised = false;
    const std::vector<FlagGroup*> _groups;
    std::vector<Flag*> _flags;
};

class Flag
{
public:
    Flag(const std::vector<std::string>& names, const std::string helptext);
    virtual ~Flag(){};

    void checkInitialised() const
    {
        _group.checkInitialised();
    }

    const std::string& name() const
    {
        return _names[0];
    }
    const std::vector<std::string> names() const
    {
        return _names;
    }
    const std::string& helptext() const
    {
        return _helptext;
    }

    virtual bool hasArgument() const = 0;
    virtual const std::string defaultValueAsString() const = 0;
    virtual void set(const std::string& value) = 0;

private:
    FlagGroup& _group;
    const std::vector<std::string> _names;
    const std::string _helptext;
};

class ActionFlag : Flag
{
public:
    ActionFlag(const std::vector<std::string>& names,
        const std::string helptext,
        std::function<void(void)> callback):
        Flag(names, helptext),
        _callback(callback)
    {
    }

    bool hasArgument() const
    {
        return false;
    }
    const std::string defaultValueAsString() const
    {
        return "";
    }
    void set(const std::string& value)
    {
        _callback();
    }

private:
    const std::function<void(void)> _callback;
};

class SettableFlag : public Flag
{
public:
    SettableFlag(
        const std::vector<std::string>& names, const std::string helptext):
        Flag(names, helptext)
    {
    }

    operator bool() const
    {
        checkInitialised();
        return _value;
    }

    bool hasArgument() const
    {
        return false;
    }
    const std::string defaultValueAsString() const
    {
        return "false";
    }
    void set(const std::string& value)
    {
        _value = true;
    }

private:
    bool _value = false;
};

template <typename T>
class ValueFlag : public Flag
{
public:
    ValueFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        const T defaultValue,
        std::function<void(const T&)> callback =
            [](const T&)
        {
        }):
        Flag(names, helptext),
        _defaultValue(defaultValue),
        _value(defaultValue),
        _callback(callback)
    {
    }

    const T& get() const
    {
        checkInitialised();
        return _value;
    }

    operator const T&() const
    {
        return get();
    }

    bool isSet() const
    {
        return _isSet;
    }

    void setDefaultValue(T value)
    {
        _value = _defaultValue = value;
    }

    bool hasArgument() const
    {
        return true;
    }

protected:
    T _defaultValue;
    T _value;
    bool _isSet = false;
    std::function<void(const T&)> _callback;
};

class StringFlag : public ValueFlag<std::string>
{
public:
    StringFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        const std::string defaultValue = "",
        std::function<void(const std::string&)> callback =
            [](const std::string&)
        {
        }):
        ValueFlag(names, helptext, defaultValue, callback)
    {
    }

    const std::string defaultValueAsString() const
    {
        return _defaultValue;
    }
    void set(const std::string& value)
    {
        _value = value;
        _callback(_value);
        _isSet = true;
    }
};

class IntFlag : public ValueFlag<int>
{
public:
    IntFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        int defaultValue = 0,
        std::function<void(const int&)> callback =
            [](const int&)
        {
        }):
        ValueFlag(names, helptext, defaultValue, callback)
    {
    }

    const std::string defaultValueAsString() const
    {
        return std::to_string(_defaultValue);
    }
    void set(const std::string& value)
    {
        _value = std::stoi(value);
        _callback(_value);
        _isSet = true;
    }
};

class HexIntFlag : public IntFlag
{
public:
    HexIntFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        int defaultValue = 0,
        std::function<void(const int&)> callback =
            [](const int&)
        {
        }):
        IntFlag(names, helptext, defaultValue, callback)
    {
    }

    const std::string defaultValueAsString() const;
};

class DoubleFlag : public ValueFlag<double>
{
public:
    DoubleFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        double defaultValue = 1.0,
        std::function<void(const double&)> callback =
            [](const double&)
        {
        }):
        ValueFlag(names, helptext, defaultValue, callback)
    {
    }

    const std::string defaultValueAsString() const
    {
        return std::to_string(_defaultValue);
    }
    void set(const std::string& value)
    {
        _value = std::stod(value);
        _callback(_value);
        _isSet = true;
    }
};

class BoolFlag : public ValueFlag<bool>
{
public:
    BoolFlag(
        const std::vector<std::string>& names,
        const std::string helptext,
        bool defaultValue = false,
        std::function<void(const bool&)> callback =
            [](const bool&)
        {
        }):
        ValueFlag(names, helptext, defaultValue, callback)
    {
    }

    const std::string defaultValueAsString() const
    {
        return _defaultValue ? "true" : "false";
    }
    void set(const std::string& value);
};

#endif
