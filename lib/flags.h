#ifndef FLAGS_H
#define FLAGS_H

class Flag
{
public:
    static void parseFlags(int argc, const char* argv[]);

    Flag(const std::vector<std::string>& names, const std::string helptext);
    virtual ~Flag() {};

    const std::string& name() const { return _names[0]; }
    const std::vector<std::string> names() const { return _names; }
    const std::string& helptext() const { return _helptext; }

    virtual bool hasArgument() const = 0;
    virtual const std::string defaultValue() const = 0;
    virtual void set(const std::string value) = 0;

private:
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
    const std::string defaultValue() const { return ""; }
    void set(const std::string value) { _callback(); }

private:
    const std::function<void(void)> _callback;
};

class SettableFlag : public Flag
{
public:
    SettableFlag(const std::vector<std::string>& names, const std::string helptext):
        Flag(names, helptext)
    {}

    operator bool() const { return _value; }

    bool hasArgument() const { return false; }
    const std::string defaultValue() const { return "false"; }
    void set(const std::string value) { _value = true; }

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

    T value() const { return _value; }
    operator T() const { return _value; }

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

    const std::string defaultValue() const { return _defaultValue; }
    void set(const std::string value) { _value = value; }
};

class IntFlag : public ValueFlag<int>
{
public:
    IntFlag(const std::vector<std::string>& names, const std::string helptext,
            int defaultValue = 0):
        ValueFlag(names, helptext, defaultValue)
    {}

    const std::string defaultValue() const { return std::to_string(_defaultValue); }
    void set(const std::string value) { _value = std::stoi(value); }
};

#endif
