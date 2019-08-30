#ifndef FLAGS_H
#define FLAGS_H

class DataSpec;
class Flag;

class FlagGroup
{
private:
    FlagGroup(const FlagGroup& group);
public:
    FlagGroup(const std::initializer_list<FlagGroup*> groups);
    FlagGroup();

public:
    void parseFlags(int argc, const char* argv[]);
    std::vector<std::string> parseFlagsWithFilenames(int argc, const char* argv[]);
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
    virtual ~Flag() {};

    void checkInitialised() const
    { _group.checkInitialised(); }

    const std::string& name() const { return _names[0]; }
    const std::vector<std::string> names() const { return _names; }
    const std::string& helptext() const { return _helptext; }

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
    ActionFlag(const std::vector<std::string>& names, const std::string helptext,
            std::function<void(void)> callback):
        Flag(names, helptext),
        _callback(callback)
    {}

    bool hasArgument() const { return false; }
    const std::string defaultValueAsString() const { return ""; }
    void set(const std::string& value) { _callback(); }

private:
    const std::function<void(void)> _callback;
};

class SettableFlag : public Flag
{
public:
    SettableFlag(const std::vector<std::string>& names, const std::string helptext):
        Flag(names, helptext)
    {}

    operator bool() const
    { checkInitialised(); return _value; }

    bool hasArgument() const { return false; }
    const std::string defaultValueAsString() const { return "false"; }
    void set(const std::string& value) { _value = true; }

private:
    bool _value = false;
};

template <typename T>
class ValueFlag : public Flag
{
public:
    ValueFlag(const std::vector<std::string>& names, const std::string helptext,
            const T defaultValue):
        Flag(names, helptext),
        _defaultValue(defaultValue),
        _value(defaultValue)
    {}

    const T& get() const
    { checkInitialised(); return _value; }

    operator const T& () const 
    { return get(); }

    void setDefaultValue(T value)
    {
        _value = _defaultValue = value;
    }

    bool hasArgument() const { return true; }

protected:
    T _defaultValue;
    T _value;
};

class StringFlag : public ValueFlag<std::string>
{
public:
    StringFlag(const std::vector<std::string>& names, const std::string helptext,
            const std::string defaultValue = ""):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return _defaultValue; }
    void set(const std::string& value) { _value = value; }
};

class IntFlag : public ValueFlag<int>
{
public:
    IntFlag(const std::vector<std::string>& names, const std::string helptext,
            int defaultValue = 0):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return std::to_string(_defaultValue); }
    void set(const std::string& value) { _value = std::stoi(value); }
};

class DoubleFlag : public ValueFlag<double>
{
public:
    DoubleFlag(const std::vector<std::string>& names, const std::string helptext,
            double defaultValue = 1.0):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return std::to_string(_defaultValue); }
    void set(const std::string& value) { _value = std::stod(value); }
};

class BoolFlag : public ValueFlag<double>
{
public:
    BoolFlag(const std::vector<std::string>& names, const std::string helptext,
            bool defaultValue = false):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValueAsString() const { return _defaultValue ? "true" : "false"; }
    void set(const std::string& value);
};

#endif
